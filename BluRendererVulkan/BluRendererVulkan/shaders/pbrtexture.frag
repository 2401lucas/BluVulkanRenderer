#version 450

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec4 inTangent;

layout (binding = 0) uniform UBO {
	mat4 projection;
	mat4 model;
	mat4 view;
	vec3 camPos;
} ubo;

struct LightInfo {
	vec4 pos;	//XYZ for position, W for light type
	vec4 rot;	//XYZ  for rotation
	vec4 color;	//XYZ for RGB, W for Intensity
};

layout (binding = 1) uniform UBOParams {
	LightInfo lights[1];
	float exposure;
	float gamma;
} uboParams;

//Prebaked Scene Resources
//Sampler for Environtment Map
layout (binding = 2) uniform samplerCube samplerIrradiance;
//Sampler for Irradiance
layout (binding = 3) uniform sampler2D samplerBRDFLUT;
//Environtment Map
layout (binding = 4) uniform samplerCube prefilteredMap;

//PBR Textures
layout (binding = 5) uniform sampler2D albedoMap;
layout (binding = 6) uniform sampler2D normalMap;
layout (binding = 7) uniform sampler2D aoMap;
layout (binding = 8) uniform sampler2D metallicMap;
layout (binding = 9) uniform sampler2D roughnessMap;


layout (location = 0) out vec4 outColor;

#define PI 3.1415926535897932384626433832795
// Applies Gamma correction
#define ALBEDO pow(texture(albedoMap, inUV).rgb, vec3(2.2))

// Normal Distribution function --------------------------------------
float DistributionGGX(float dotNH, float roughness)
{
	float alpha = roughness * roughness;
	float alpha2 = alpha * alpha;
	float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
	return (alpha2)/(PI * denom*denom); 
}

// Geometric Shadowing function --------------------------------------
float GeometrySchlickGGX(float dotNL, float dotNV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r*r) / 8.0;
	float GL = dotNL / (dotNL * (1.0 - k) + k);
	float GV = dotNV / (dotNV * (1.0 - k) + k);
	return GL * GV;
}

// Fresnel function ----------------------------------------------------
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}
vec3 FresnelSchlickR(float cosTheta, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 prefilteredReflection(vec3 R, float roughness)
{
	const float MAX_REFLECTION_LOD = 9.0; // todo: param/const
	float lod = roughness * MAX_REFLECTION_LOD;
	float lodf = floor(lod);
	float lodc = ceil(lod);
	vec3 a = textureLod(prefilteredMap, R, lodf).rgb;
	vec3 b = textureLod(prefilteredMap, R, lodc).rgb;
	return mix(a, b, lod - lodf);
}

vec3 specularContribution(vec3 L, vec3 V, vec3 N, vec3 F0, vec3 radiance, float metallic, float roughness)
{
	// Precalculate vectors and dot products	
	vec3 H = normalize (V + L);
	float dotNH = clamp(dot(N, H), 0.0, 1.0);
	float dotNV = clamp(dot(N, V), 0.0, 1.0);
	float dotNL = clamp(dot(N, L), 0.0, 1.0);
	
	vec3 color = vec3(0.0);

	if (dotNL > 0.0) {
		// D = Normal distribution (Distribution of the microfacets)
		float D = DistributionGGX(dotNH, roughness); 
		// G = Geometric shadowing term (Microfacets shadowing)
		float G = GeometrySchlickGGX(dotNL, dotNV, roughness);
		// F = Fresnel factor (Reflectance depending on angle of incidence)
		vec3 F = FresnelSchlick(dotNV, F0);		
		vec3 spec = D * F * G / (4.0 * dotNL * dotNV + 0.001);		
		vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);			
		color += (kD * ALBEDO / PI + spec) * radiance * dotNL;
	}

	return color;
}

vec3 calculateNormal()
{
	vec3 tangentNormal = texture(normalMap, inUV).xyz * 2.0 - 1.0;

	vec3 N = normalize(inNormal);
	vec3 T = normalize(inTangent.xyz);
	vec3 B = normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);
	return normalize(TBN * tangentNormal);
}

void main()
{		
	vec3 N = calculateNormal();

	vec3 V = normalize(ubo.camPos - inWorldPos);
	vec3 R = reflect(-V, N); 

	float metallic = texture(metallicMap, inUV).r;
	float roughness = texture(roughnessMap, inUV).r;

	vec3 F0 = vec3(0.04); 
	F0 = mix(F0, ALBEDO, metallic);

	//Lights
	vec3 Lo = vec3(0.0);
	for(int i = 0; i < uboParams.lights.length(); i++) {
		float distance    = length(uboParams.lights[i].pos.xyz - inWorldPos);
		float attenuation = 1.0 / (distance * distance);
		vec3 radiance     = uboParams.lights[i].color.xyz * attenuation;
		
		vec3 L = normalize(uboParams.lights[i].pos.xyz - inWorldPos);

		Lo += specularContribution(L, V, N, F0, radiance, metallic, roughness);
	}
	
	vec2 brdf = texture(samplerBRDFLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
	vec3 reflection = prefilteredReflection(R, roughness).rgb;
	vec3 irradiance = texture(samplerIrradiance, N).rgb;

	// Diffuse based on irradiance
	vec3 diffuse = irradiance * ALBEDO;

	vec3 F = FresnelSchlickR(max(dot(N, V), 0.0), F0, roughness);

	// Specular reflectance
	vec3 specular = reflection * (F * brdf.x + brdf.y);

	// Ambient part
	vec3 kD = 1.0 - F;
	kD *= 1.0 - metallic;
	vec3 ambient = (kD * diffuse + specular) * texture(aoMap, inUV).rrr;
	
	vec3 color = (ambient + Lo) * uboParams.exposure;

	outColor = vec4(color, 1.0);
}