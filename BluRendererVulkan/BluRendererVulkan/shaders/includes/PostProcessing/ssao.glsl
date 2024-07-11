#ifndef SSAO
    #define SSAO 1
#endif

vec3 calcViewPosition(vec2 uv, float depth) {
    //float z = ubo.zFar * ubo.zNear / (ubo.zFar + depth * (ubo.zNear - ubo.zFar));
    //float z = depth * 2.0 - 1.0;
    float z = depth;
    vec4 clipSpacePosition = vec4(uv * 2.0 - 1, z, 1.0);
    vec4 viewSpacePosition = ubo.inverseProj * clipSpacePosition;
    // Perspective division
    viewSpacePosition /= viewSpacePosition.w;

    return viewSpacePosition.xyz;
}
vec3 calcWorldPosition(vec2 uv, float depth) {
    //float z = ubo.zFar * ubo.zNear / (ubo.zFar + depth * (ubo.zNear - ubo.zFar));
    //float z = depth * 2.0 - 1.0;
    float z = depth;
    vec4 clipSpacePosition = vec4(uv * 2.0 - 1, z, 1.0);
    vec4 viewSpacePosition = ubo.inverseProj * clipSpacePosition;
    // Perspective division
    viewSpacePosition /= viewSpacePosition.w;
    vec4 worldSpacePosition = ubo.inverseView * viewSpacePosition;

    return worldSpacePosition.xyz;
}


vec3 ApplySSAO() {
    float depth = texture(depthTexture, fragTexCoord).r;
    vec3 worldPosition = calcWorldPosition(fragTexCoord, depth);
    vec3 normal = normalize(cross(dFdx(worldPosition), dFdy(worldPosition)));
	normal.y *= -1.0f;

    vec3 centerDepthPos = calcViewPosition(fragTexCoord, depth);

    vec3 rvec = vec3(texture(randomTexture, fragTexCoord * ubo.noiseScale).xy, 0);
    vec3 tangent = normalize(rvec - normal * dot(rvec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 tbn = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    for (int i = 0; i < int(ubo.kernelSize); ++i) 
    {
        int index = i * 3;
        float x = aoSampling.mSSAOKernel[index / 4][index % 4];
        float y = aoSampling.mSSAOKernel[(index+1) / 4][(index+1) % 4];
        float z = aoSampling.mSSAOKernel[(index+2) / 4][(index+2) % 4];

	    vec3 samplePos = vec3(x,y,z) * tbn;
	    samplePos = samplePos * ubo.radius + centerDepthPos;
	    
        vec4 offset = vec4(samplePos, 1.0);
        offset = ubo.proj * offset;    // from view to clip-space
        offset.xyz /= offset.w;               // perspective divide
        offset.xy  = offset.xy * 0.5 + 0.5; // transform to range 0.0 - 1.0

        float sampleDepth = calcViewPosition(offset.xy, texture(depthTexture, offset.xy).r).z;
        
        float rangeCheck = smoothstep(0.0, 1.0, ubo.radius / abs(centerDepthPos.z - sampleDepth));
        float bias = 0.025f;
        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
    }
    
    occlusion = 1.0 - (occlusion / ubo.kernelSize);
    return vec3(occlusion);
}