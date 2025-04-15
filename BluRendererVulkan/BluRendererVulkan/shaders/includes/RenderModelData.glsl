struct RenderModelData
{
	int vert_offset;
	uint ind_count;
	uint ind_offset;
	uint material_type;  
	int base_tex_id;
	int normal_tex_id;
	int emission_tex_id;
	int metalness_tex_id;
	int diffuse_roughness_id;
	int ambient_occlusion_id;
};

layout(buffer_reference, std430) buffer RenderModelDataBuffer {
	RenderModelData data[];
};