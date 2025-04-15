struct SceneModelData
{
	vec4 sphere_bounding_box;
	uint id;
	
	uint pad1;
	uint pad2;
	uint pad3;
};

layout(buffer_reference, std430) buffer SceneModelDataBuffer {
	vec4 planes[6];
	SceneModelData model_data[];
};