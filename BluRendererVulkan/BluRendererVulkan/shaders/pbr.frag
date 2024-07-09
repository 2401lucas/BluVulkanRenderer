#version 450

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV0;
layout (location = 3) in vec2 inUV1;
layout (location = 4) in vec4 inColor0;
layout (location = 5) in vec4 inShadowCoords;

// Scene bindings

layout (set = 0, binding = 0) uniform UBO {
	mat4 model[16];
	mat4 projection;
	mat4 view;
	mat4 lightSpace;
	vec3 camPos;
} ubo;

 struct LightSource {
    vec4 color;
    vec4 position;
    vec4 dir;
    float zNear;
    float zFar;
    float lightFOV;
	float lightType;
  };

layout (set = 0, binding = 1) uniform UBOParams {
	LightSource lights[2];
	float prefilteredCubeMipLevels;
	float debugViewInputs;
	float debugViewLight;
	float scaleIBLAmbient;
} uboParams;

layout (set = 0, binding = 2) uniform samplerCube samplerIrradiance;
layout (set = 0, binding = 3) uniform samplerCube prefilteredMap;
layout (set = 0, binding = 4) uniform sampler2D samplerBRDFLUT;
layout (set = 0, binding = 5) uniform sampler2D shadowMap;

// Material bindings

// Textures

layout (set = 1, binding = 0) uniform sampler2D colorMap;
layout (set = 1, binding = 1) uniform sampler2D physicalDescriptorMap;
layout (set = 1, binding = 2) uniform sampler2D normalMap;
layout (set = 1, binding = 3) uniform sampler2D aoMap;
layout (set = 1, binding = 4) uniform sampler2D emissiveMap;

// Properties

#include "includes/shaderMaterial.glsl"

layout(std430, set = 3, binding = 0) buffer SSBO
{
   ShaderMaterial materials[ ];
};

layout (push_constant) uniform PushConstants {
	int materialIndex;
	int transformIndex;
} pushConstants;

layout (location = 0) out vec4 outColor;

struct PBRInfo
{
	vec3 N;
	vec3 V;
	float NdotL;                  // cos angle between normal and light direction
	float NdotV;                  // cos angle between normal and view direction
	float NdotH;                  // cos angle between normal and half vector
	float LdotH;                  // cos angle between light direction and half vector
	float VdotH;                  // cos angle between view direction and half vector
	float perceptualRoughness;    // roughness value, as authored by the model creator (input to shader)
	float metalness;              // metallic value at the surface
	vec3 reflectance0;            // full reflectance color (normal incidence angle)
	vec3 reflectance90;           // reflectance color at grazing angle
	float alphaRoughness;         // roughness mapped to a more linear change in the roughness (proposed by [2])
	vec3 diffuseColor;            // color contribution from diffuse lighting
	vec3 specularColor;           // color contribution from specular lighting
};

const float M_PI = 3.141592653589793;
const float c_MinRoughness = 0.04;

const float PBR_WORKFLOW_METALLIC_ROUGHNESS = 0.0;
const float PBR_WORKFLOW_SPECULAR_GLOSSINESS = 1.0;

#include "includes/srgbtolinear.glsl"

// Find the normal for this fragment, pulling either from a predefined normal map
// or from the interpolated mesh normal and tangent attributes.
vec3 getNormal(ShaderMaterial material)
{
	// Perturb normal, see http://www.thetenthplanet.de/archives/1180
	vec3 tangentNormal = texture(normalMap, material.normalTextureSet == 0 ? inUV0 : inUV1).xyz * 2.0 - 1.0;

	vec3 q1 = dFdx(inWorldPos);
	vec3 q2 = dFdy(inWorldPos);
	vec2 st1 = dFdx(inUV0);
	vec2 st2 = dFdy(inUV0);

	vec3 N = normalize(inNormal);
	vec3 T = normalize(q1 * st2.t - q2 * st1.t);
	vec3 B = -normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);

	return normalize(TBN * tangentNormal);
}

vec3 getIBLContribution(PBRInfo pbrInputs, vec3 reflection)
{
	float lod = (pbrInputs.perceptualRoughness * uboParams.prefilteredCubeMipLevels);
	vec3 brdf = texture(samplerBRDFLUT, clamp(vec2(pbrInputs.NdotV, 1.0 - pbrInputs.perceptualRoughness), vec2(0), vec2(1.0))).rgb;
	vec3 diffuseLight = texture(samplerIrradiance, pbrInputs.N).rgb;
	vec3 specularLight = textureLod(prefilteredMap, reflection, lod).rgb;

	vec3 diffuse = diffuseLight * pbrInputs.diffuseColor;
	vec3 specular = specularLight * (pbrInputs.specularColor * brdf.x + brdf.y);

	// For presentation, this allows us to disable IBL terms
	diffuse *= uboParams.scaleIBLAmbient;
	specular *= uboParams.scaleIBLAmbient;
	return diffuse + specular;
}

vec3 diffuse(PBRInfo pbrInputs)
{
	return pbrInputs.diffuseColor / M_PI;
}

// The following equation models the Fresnel reflectance term of the spec equation (aka F())
// Implementation of fresnel from [4], Equation 15
vec3 specularReflection(PBRInfo pbrInputs)
{
	return pbrInputs.reflectance0 + (pbrInputs.reflectance90 - pbrInputs.reflectance0) * pow(clamp(1.0 - pbrInputs.VdotH, 0.0, 1.0), 5.0);
}

// This calculates the specular geometric attenuation (aka G()),
// where rougher material will reflect less light back to the viewer.
// This implementation is based on [1] Equation 4, and we adopt their modifications to
// alphaRoughness as input as originally proposed in [2].
float geometricOcclusion(PBRInfo pbrInputs)
{
	float NdotL = pbrInputs.NdotL;
	float NdotV = pbrInputs.NdotV;
	float r = pbrInputs.alphaRoughness;

	float attenuationL = 2.0 * NdotL / (NdotL + sqrt(r * r + (1.0 - r * r) * (NdotL * NdotL)));
	float attenuationV = 2.0 * NdotV / (NdotV + sqrt(r * r + (1.0 - r * r) * (NdotV * NdotV)));
	return attenuationL * attenuationV;
}

// The following equation(s) model the distribution of microfacet normals across the area being drawn (aka D())
// Implementation from "Average Irregularity Representation of a Roughened Surface for Ray Reflection" by T. S. Trowbridge, and K. P. Reitz
// Follows the distribution function recommended in the SIGGRAPH 2013 course notes from EPIC Games [1], Equation 3.
float microfacetDistribution(PBRInfo pbrInputs)
{
	float roughnessSq = pbrInputs.alphaRoughness * pbrInputs.alphaRoughness;
	float f = (pbrInputs.NdotH * roughnessSq - pbrInputs.NdotH) * pbrInputs.NdotH + 1.0;
	return roughnessSq / (M_PI * f * f);
	//return roughnessSq / (M_PI * pow(pow(pbrInputs.NdotH, 2) * (roughnessSq - 1) + 1, 2));
}

float calculateShadow(vec4 shadowCoords, vec2 off)
{
   if (shadowCoords.z > -1.0 && shadowCoords.z < 1.0)
   {
      float closestDepth = texture(shadowMap, shadowCoords.xy + off).r;
      float currentDepth = shadowCoords.z;

      if (closestDepth < currentDepth)
         return 1.0;
   }

   return 0.0;
}

float filterPCF(vec4 shadowCoords)
{
   vec2 texelSize = textureSize(shadowMap, 0);
   float scale = 1.5;
   float dx = scale * (1.0 / float(texelSize.x));
   float dy = scale * (1.0 / float(texelSize.y));

   float shadow = 0.0;
   int count = 0;
   int range = 1;

   for (int x = -range; x <= range; x++)
   {
      for (int y = -range; y <= range; y++)
      {
         shadow += calculateShadow(
               shadowCoords,
               vec2(dx * x, dy * y)
         );
         count++;
      }
   }

   return shadow / count;
}

// Gets metallic factor from specular glossiness workflow inputs 
float convertMetallic(vec3 diffuse, vec3 specular, float maxSpecular) {
	float perceivedDiffuse = sqrt(0.299 * diffuse.r * diffuse.r + 0.587 * diffuse.g * diffuse.g + 0.114 * diffuse.b * diffuse.b);
	float perceivedSpecular = sqrt(0.299 * specular.r * specular.r + 0.587 * specular.g * specular.g + 0.114 * specular.b * specular.b);
	if (perceivedSpecular < c_MinRoughness) {
		return 0.0;
	}
	float a = c_MinRoughness;
	float b = perceivedDiffuse * (1.0 - maxSpecular) / (1.0 - c_MinRoughness) + perceivedSpecular - 2.0 * c_MinRoughness;
	float c = c_MinRoughness - perceivedSpecular;
	float D = max(b * b - 4.0 * a * c, 0.0);
	return clamp((-b + sqrt(D)) / (2.0 * a), 0.0, 1.0);
}

vec3 CalculateDirLight(LightSource light, PBRInfo pbrInputs) {
	// Precalculate vectors and dot products	
	vec3 L = normalize(light.dir.xyz);
	vec3 H = normalize (pbrInputs.V + L);
	pbrInputs.NdotL = clamp(dot(pbrInputs.N, L), 0.001, 1.0);
	pbrInputs.NdotH = clamp(dot(pbrInputs.N, H), 0.0, 1.0);
	pbrInputs.LdotH = clamp(dot(L, H), 0.0, 1.0);
	pbrInputs.VdotH = clamp(dot(pbrInputs.V, H), 0.0, 1.0);

	vec3 inRadiance = light.color.w * light.color.rbg;

	// D = Normal distribution (Distribution of the microfacets)
	float D = microfacetDistribution(pbrInputs); 
	// G = Geometric shadowing term (Microfacets shadowing)
	float G = geometricOcclusion(pbrInputs);
	// F = Fresnel factor (Reflectance depending on angle of incidence)
	vec3 F = specularReflection(pbrInputs);

	// Energy conservation
	// Specular and Diffuse
	vec3 kS = F;
	vec3 kD = vec3(1.0) - kS;
	kD *= 1.0 - pbrInputs.metalness;

	vec3 numerator = D * G * F;
	float denominator = 4.0 * pbrInputs.NdotV * pbrInputs.NdotL;

	vec3 diffuse = kD * diffuse(pbrInputs);
	vec3 specular = numerator / max(denominator, 0.0001);

	return ((diffuse + specular) * inRadiance * pbrInputs.NdotL);
}

vec3 CalculatePointLight(LightSource light, PBRInfo pbrInputs) {
	// Precalculate vectors and dot products	
	vec3 L = normalize(light.position.xyz - inWorldPos);
	vec3 H = normalize (pbrInputs.V + L);
	pbrInputs.NdotL = clamp(dot(pbrInputs.N, L), 0.001, 1.0);
	pbrInputs.NdotH = clamp(dot(pbrInputs.N, H), 0.0, 1.0);
	pbrInputs.LdotH = clamp(dot(L, H), 0.0, 1.0);
	pbrInputs.VdotH = clamp(dot(pbrInputs.V, H), 0.0, 1.0);

	vec3 inRadiance = light.color.w * light.color.rbg;

	// D = Normal distribution (Distribution of the microfacets)
	float D = microfacetDistribution(pbrInputs); 
	// G = Geometric shadowing term (Microfacets shadowing)
	float G = geometricOcclusion(pbrInputs);
	// F = Fresnel factor (Reflectance depending on angle of incidence)
	vec3 F = specularReflection(pbrInputs);

	// Energy conservation
	// Specular and Diffuse
	vec3 kS = F;
	vec3 kD = vec3(1.0) - kS;
	kD *= 1.0 - pbrInputs.metalness;

	vec3 numerator = D * G * F;
	float denominator = 4.0 * pbrInputs.NdotV * pbrInputs.NdotL;

	vec3 diffuse = kD * diffuse(pbrInputs);
	vec3 specular = numerator / max(denominator, 0.0001);

	// TODO: Make these const. adjustable by the GUI.
	float lightConst = 1.0;
	float lightLinear = 0.09;
	float lightQuadratic = 0.032;
   
	float distance = length(light.position.xyz - inWorldPos);
	float attenuation = (1.0 / ( lightConst + lightLinear * distance + lightQuadratic * (distance * distance)));

	return (attenuation * (diffuse + specular) * inRadiance * pbrInputs.NdotL);
}

vec3 CalculateSpotLight(LightSource light, PBRInfo pbrInputs) {
	// Precalculate vectors and dot products	
	vec3 L = normalize(light.position.xyz - inWorldPos);
	vec3 H = normalize (pbrInputs.V + L);
	pbrInputs.NdotL = clamp(dot(pbrInputs.N, L), 0.001, 1.0);
	pbrInputs.NdotH = clamp(dot(pbrInputs.N, H), 0.0, 1.0);
	pbrInputs.LdotH = clamp(dot(L, H), 0.0, 1.0);
	pbrInputs.VdotH = clamp(dot(pbrInputs.V, H), 0.0, 1.0);

	float theta = dot(L, normalize(-light.dir.xyz));
   float epsilon = 0.9978 - light.lightFOV;
   float intensity = clamp((theta - light.lightFOV) / epsilon, 0.0, 1.0);

	vec3 inRadiance = light.color.w * light.color.rbg;

	// D = Normal distribution (Distribution of the microfacets)
	float D = microfacetDistribution(pbrInputs); 
	// G = Geometric shadowing term (Microfacets shadowing)
	float G = geometricOcclusion(pbrInputs);
	// F = Fresnel factor (Reflectance depending on angle of incidence)
	vec3 F = specularReflection(pbrInputs);

	// Energy conservation
	// Specular and Diffuse
	vec3 kS = F;
	vec3 kD = vec3(1.0) - kS;
	kD *= 1.0 - pbrInputs.metalness;

	vec3 numerator = D * G * F;
	float denominator = 4.0 * pbrInputs.NdotV * pbrInputs.NdotL;

	vec3 diffuse = kD * diffuse(pbrInputs) * intensity;
	vec3 specular = numerator / max(denominator, 0.0001) * intensity;

	// TODO: Make these const. adjustable by the GUI.
	float lightConst = 1.0;
	float lightLinear = 0.09;
	float lightQuadratic = 0.032;
   
	float distance = length(light.position.xyz - inWorldPos);
	float attenuation = (1.0 / ( lightConst + lightLinear * distance + lightQuadratic * (distance * distance)));

	return (attenuation * (diffuse + specular) * inRadiance * pbrInputs.NdotL);
}

void main()
{
	ShaderMaterial material = materials[pushConstants.materialIndex];

	float perceptualRoughness;
	float metallic;
	vec4 baseColor;

	vec3 f0 = vec3(0.04);

	if (material.alphaMask == 1.0f) {
		if (material.baseColorTextureSet > -1) {
			baseColor = SRGBtoLINEAR(texture(colorMap, material.baseColorTextureSet == 0 ? inUV0 : inUV1)) * material.baseColorFactor;
		} else {
			baseColor = material.baseColorFactor;
		}
		if (baseColor.a < material.alphaMaskCutoff) {
			discard;
		}
	}

	if (material.workflow == PBR_WORKFLOW_METALLIC_ROUGHNESS) {
		// Metallic and Roughness material properties are packed together
		// In glTF, these factors can be specified by fixed scalar values
		// or from a metallic-roughness map
		perceptualRoughness = material.roughnessFactor;
		metallic = material.metallicFactor;
		if (material.physicalDescriptorTextureSet > -1) {
			// Roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
			// This layout intentionally reserves the 'r' channel for (optional) occlusion map data
			vec4 mrSample = texture(physicalDescriptorMap, material.physicalDescriptorTextureSet == 0 ? inUV0 : inUV1);
			perceptualRoughness = mrSample.g * perceptualRoughness;
			metallic = mrSample.b * metallic;
		} else {
			perceptualRoughness = clamp(perceptualRoughness, c_MinRoughness, 1.0);
			metallic = clamp(metallic, 0.0, 1.0);
		}
		// Roughness is authored as perceptual roughness; as is convention,
		// convert to material roughness by squaring the perceptual roughness [2].

		// The albedo may be defined from a base texture or a flat color
		if (material.baseColorTextureSet > -1) {
			baseColor = SRGBtoLINEAR(texture(colorMap, material.baseColorTextureSet == 0 ? inUV0 : inUV1)) * material.baseColorFactor;
		} else {
			baseColor = material.baseColorFactor;
		}
	}

	if (material.workflow == PBR_WORKFLOW_SPECULAR_GLOSSINESS) {
		// Values from specular glossiness workflow are converted to metallic roughness
		if (material.physicalDescriptorTextureSet > -1) {
			perceptualRoughness = 1.0 - texture(physicalDescriptorMap, material.physicalDescriptorTextureSet == 0 ? inUV0 : inUV1).a;
		} else {
			perceptualRoughness = 0.0;
		}

		const float epsilon = 1e-6;

		vec4 diffuse = SRGBtoLINEAR(texture(colorMap, inUV0));
		vec3 specular = SRGBtoLINEAR(texture(physicalDescriptorMap, inUV0)).rgb;

		float maxSpecular = max(max(specular.r, specular.g), specular.b);

		// Convert metallic value from specular glossiness inputs
		metallic = convertMetallic(diffuse.rgb, specular, maxSpecular);

		vec3 baseColorDiffusePart = diffuse.rgb * ((1.0 - maxSpecular) / (1 - c_MinRoughness) / max(1 - metallic, epsilon)) * material.diffuseFactor.rgb;
		vec3 baseColorSpecularPart = specular - (vec3(c_MinRoughness) * (1 - metallic) * (1 / max(metallic, epsilon))) * material.specularFactor.rgb;
		baseColor = vec4(mix(baseColorDiffusePart, baseColorSpecularPart, metallic * metallic), diffuse.a);

	}

	baseColor *= inColor0;

	vec3 diffuseColor = baseColor.rgb * (vec3(1.0) - f0);
	diffuseColor *= 1.0 - metallic;
	vec3 specularColor = mix(f0, baseColor.rgb, metallic);
	float alphaRoughness = perceptualRoughness * perceptualRoughness;

	// Compute reflectance.
	float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);
	// For typical incident reflectance range (between 4% to 100%) set the grazing reflectance to 100% for typical fresnel effect.
	// For very low reflectance range on highly diffuse objects (below 4%), incrementally reduce grazing reflecance to 0%.
	float reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);
	vec3 specularEnvironmentR0 = specularColor.rgb;
	vec3 specularEnvironmentR90 = vec3(1.0, 1.0, 1.0) * reflectance90;

	vec3 n = (material.normalTextureSet > -1) ? getNormal(material) : normalize(inNormal);
	n.y *= -1.0f;
	vec3 v = normalize(ubo.camPos - inWorldPos);    // Vector from surface point to camera
	//vec3 l = normalize(-inWorldPos - vec3(0.0,10.0,0.0));     // Vector from surface point to light
	vec3 l = vec3(0.0f);
	//vec3 h = normalize(l+v);                        // Half vector between both l and v
	vec3 h = vec3(0.0f);
	vec3 reflection = normalize(reflect(-v, n));

	float NdotL = clamp(dot(n, l), 0.001, 1.0);
	float NdotV = clamp(abs(dot(n, v)), 0.001, 1.0);
	float NdotH = clamp(dot(n, h), 0.0, 1.0);
	float LdotH = clamp(dot(l, h), 0.0, 1.0);
	float VdotH = clamp(dot(v, h), 0.0, 1.0);
	
	PBRInfo pbrInputs = PBRInfo(
		n,
		v,
		NdotL,
		NdotV,
		NdotH,
		LdotH,
		VdotH,
		perceptualRoughness,
		metallic,
		specularEnvironmentR0,
		specularEnvironmentR90,
		alphaRoughness,
		diffuseColor,
		specularColor
	);

	vec3 color = vec3(0.0);

	//Lights
	vec3 ibl = getIBLContribution(pbrInputs, reflection);
	color += ibl;

	vec3 Lo = vec3(0.0);
	for(int i = 0; i < 1; i++) {
		int lightType = int(uboParams.lights[i].lightType);
		switch(lightType) {
			case 0:
			float shadow = (1.0 - filterPCF(inShadowCoords / inShadowCoords.w));
				Lo += CalculateDirLight(uboParams.lights[i], pbrInputs) * shadow;
				break;
			case 1:
				Lo += CalculatePointLight(uboParams.lights[i], pbrInputs);
				break;
			case 2:
				Lo += CalculateSpotLight(uboParams.lights[i], pbrInputs);
				break;
		}
	}

	color += Lo;
	const float u_OcclusionStrength = 1.0f;
	// Apply optional PBR terms for additional (optional) shading
	if (material.occlusionTextureSet > -1) {
		float ao = texture(aoMap, (material.occlusionTextureSet == 0 ? inUV0 : inUV1)).r;
		color = mix(color, color * ao, u_OcclusionStrength);
	}

	vec3 emissive = material.emissiveFactor.rgb * material.emissiveStrength;
	if (material.emissiveTextureSet > -1) {
		emissive *= SRGBtoLINEAR(texture(emissiveMap, material.emissiveTextureSet == 0 ? inUV0 : inUV1)).rgb;
	};
	color += emissive;


	outColor = vec4(color, baseColor.a);

	// Shader inputs debug visualization
	if (uboParams.debugViewInputs > 0.0) {
		int index = int(uboParams.debugViewInputs);
		switch (index) {
			case 1:
				outColor.rgba = material.baseColorTextureSet > -1 ? texture(colorMap, material.baseColorTextureSet == 0 ? inUV0 : inUV1) : vec4(1.0f);
				break;
			case 2:
				outColor.rgb = (material.normalTextureSet > -1) ? texture(normalMap, material.normalTextureSet == 0 ? inUV0 : inUV1).rgb : normalize(inNormal);
				break;
			case 3:
				outColor.rgb = (material.occlusionTextureSet > -1) ? texture(aoMap, material.occlusionTextureSet == 0 ? inUV0 : inUV1).rrr : vec3(0.0f);
				break;
			case 4:
				outColor.rgb = (material.emissiveTextureSet > -1) ? texture(emissiveMap, material.emissiveTextureSet == 0 ? inUV0 : inUV1).rgb : vec3(0.0f);
				break;
			case 5:
				outColor.rgb = texture(physicalDescriptorMap, inUV0).bbb;
				break;
			case 6:
				outColor.rgb = texture(physicalDescriptorMap, inUV0).ggg;
				break;
			case 7:
				vec3 debugF_L = normalize(uboParams.lights[int(uboParams.debugViewLight)].position.xyz - inWorldPos);
				vec3 debugF_H = normalize (pbrInputs.V + debugF_L);
				pbrInputs.NdotL = clamp(dot(pbrInputs.N, debugF_L), 0.001, 1.0);
				pbrInputs.NdotH = clamp(dot(pbrInputs.N, debugF_H), 0.0, 1.0);
				pbrInputs.LdotH = clamp(dot(debugF_L, debugF_H), 0.0, 1.0);
				pbrInputs.VdotH = clamp(dot(pbrInputs.V, debugF_H), 0.0, 1.0);
				// F = Fresnel factor (Reflectance depending on angle of incidence)
				vec3 F = specularReflection(pbrInputs);
				outColor.rgb = F;
				break;
			case 8:
				vec3 debugG_L = normalize(uboParams.lights[int(uboParams.debugViewLight)].position.xyz - inWorldPos);
				vec3 debugG_H = normalize (pbrInputs.V + debugG_L);
				pbrInputs.NdotL = clamp(dot(pbrInputs.N, debugG_L), 0.001, 1.0);
				pbrInputs.NdotH = clamp(dot(pbrInputs.N, debugG_H), 0.0, 1.0);
				pbrInputs.LdotH = clamp(dot(debugG_L, debugG_H), 0.0, 1.0);
				pbrInputs.VdotH = clamp(dot(pbrInputs.V, debugG_H), 0.0, 1.0);
				// G = Geometric shadowing term (Microfacets shadowing)
				float G = geometricOcclusion(pbrInputs);

				outColor.rgb = vec3(G);
				break;
			case 9: 
				vec3 debugD_L = normalize(uboParams.lights[int(uboParams.debugViewLight)].position.xyz - inWorldPos);
				vec3 debugD_H = normalize (pbrInputs.V + debugD_L);
				pbrInputs.NdotL = clamp(dot(pbrInputs.N, debugD_L), 0.001, 1.0);
				pbrInputs.NdotH = clamp(dot(pbrInputs.N, debugD_H), 0.0, 1.0);
				pbrInputs.LdotH = clamp(dot(debugD_L, debugD_H), 0.0, 1.0);
				pbrInputs.VdotH = clamp(dot(pbrInputs.V, debugD_H), 0.0, 1.0);

				// D = Normal distribution (Distribution of the microfacets)
				float D = microfacetDistribution(pbrInputs); 
				outColor.rgb = vec3(D);
				break;
			case 10:
				outColor.rgb = ibl;
				break;
			case 11:
				outColor.rgb = Lo;
				break;
			//case 1:
				//outColor.rgb = diffuseContrib;
				//break;
			//case 5:
				//outColor.rgb = specContrib;
				//break;
		}
	}
}