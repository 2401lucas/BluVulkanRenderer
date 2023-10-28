#version 450#extension GL_EXT_nonuniform_qualifier : enable//Light Types//Type 0 - Directional//Type 1 - Pointstruct LightInfo {	vec4 position; //XYZ for position, W for light type	vec4 rotation; //XYZ  for rotation	vec4 colour;	//XYZ for RGB, W for Intensity};layout(push_constant) uniform PushConstantData {    vec4 index;} pushConsts;layout(set = 1, binding = 0) uniform sampler2D[] textureSamplers;layout(location = 0) in vec3 fragColour;layout(location = 1) in vec3 fragNormal;layout(location = 2) in vec3 fragPosition;layout(location = 3) in vec2 fragTexCoord;layout(location = 0) out vec4 outColour;layout(set = 0, binding = 1) uniform GPUSceneData {	vec4 cameraPosition;	//XYZ for position, W for number of Lights	vec4 ambientColor;		//XYZ for RGB, W for Intensity	LightInfo[16] lights;} sceneData;//TODO: Convert to doing light calculations in view spacevoid main() {	vec3 norm = normalize(fragNormal);	vec3 ambLighting = sceneData.ambientColor.xyz * sceneData.ambientColor.w;	vec3 diffuse = vec3(0,0,0);	vec3 specular = vec3(0,0,0);	vec3 viewDir = normalize(sceneData.cameraPosition.xyz - fragPosition);		for (int i = 0; i < sceneData.cameraPosition.w; i++)	{		if(sceneData.lights[i].position.w == 0){		break;		}		//Directional Lighting		else if(sceneData.lights[i].position.w == 1) {			diffuse += max(dot(norm, sceneData.lights[i].rotation.xyz), 0.0) * sceneData.lights[i].colour.xyz * sceneData.lights[i].colour.w;			vec3 reflectDir = reflect(-sceneData.lights[i].rotation.xyz, norm);			specular += pow(max(dot(viewDir, reflectDir), 0.0), 32) * sceneData.lights[i].colour.xyz * sceneData.lights[i].colour.w;		}		//Point Lighting		else if(sceneData.lights[i].position.w == 2) {			vec3 lightDir = normalize(sceneData.lights[i].position.xyz - fragPosition);			diffuse += max(dot(norm, lightDir), 0.0) * sceneData.lights[i].colour.xyz * sceneData.lights[i].colour.w;			//Specular			vec3 reflectDir = reflect(-sceneData.lights[i].rotation.xyz, norm);			specular += pow(max(dot(viewDir, reflectDir), 0.0), 32) * sceneData.lights[i].colour.xyz * sceneData.lights[i].colour.w;		}	}	outColour = texture(textureSamplers[int(pushConsts.index.x)], fragTexCoord.xy) * vec4(ambLighting + diffuse + specular, 1.0);}