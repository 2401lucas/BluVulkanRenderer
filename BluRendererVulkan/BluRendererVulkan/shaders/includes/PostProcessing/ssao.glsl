#ifndef SSAO
    #define SSAO 1
#endif

//This gives the correct Z depth, but incorrect x/y;
vec3 calcViewPosition(vec2 uv, float depth) {
    float z = ubo.zFar * ubo.zNear / (ubo.zFar + depth * (ubo.zNear - ubo.zFar));
    //float z = depth * 2.0 - 1.0;
    vec4 clipSpacePosition = vec4(uv * ubo.fovScale * 2.0 - 1.0, z, 1.0);
    vec4 viewSpacePosition = ubo.inverseProj * clipSpacePosition;
    // Perspective division
    viewSpacePosition /= viewSpacePosition.w;
    vec4 worldSpacePosition = ubo.inverseView * viewSpacePosition;

    return worldSpacePosition.xyz;
}


vec3 ApplySSAO(vec3 col) {
    return col;
    
    float depth = texture(depthTexture, fragTexCoord).r;
    vec3 centerDepthPos = calcViewPosition(fragTexCoord, depth);
    return centerDepthPos;
    vec3 normal = normalize(cross(dFdx(centerDepthPos), dFdy(centerDepthPos)));
    vec3 rvec = texture(randomTexture, fragTexCoord * ubo.noiseScale).xyz;
    vec3 tangent = normalize(rvec - normal * dot(rvec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 tbn = mat3(tangent, bitangent, normal);


    float occlusion = 0.0;
    for (int i = 0; i < int(ubo.kernelSize); ++i) 
    {
        int index = i * 3;
        float x = aoSampling.mSSAOKernel[index / 4][index % 4];
        float y = aoSampling.mSSAOKernel[(index+1)/ 4][(index+1) % 4];
        float z = aoSampling.mSSAOKernel[(index+2) / 4][(index+2) % 4];
	    vec3 samplePos = vec3(x,y,z) * tbn;
	    samplePos = samplePos * ubo.radius + centerDepthPos;
	    vec3 sampleDir = normalize(samplePos - centerDepthPos);
        float nDotS = max(dot(normal, sampleDir), 0);

	    vec4 offset = vec4(samplePos, 1.0) * ubo.proj;
	    offset.xy /= offset.w;
        vec2 texOffset = vec2(offset.x * 0.5 + 0.5, -offset.y * 0.5 + 0.5);

        float sampleDepth = texture(depthTexture, texOffset).r;
	    sampleDepth = calcViewPosition(offset.xy, sampleDepth).z;

	    float rangeCheck = smoothstep(0.0, 1.0, ubo.radius / abs(centerDepthPos.z - sampleDepth));
	    occlusion += rangeCheck * step(sampleDepth, samplePos.z) * nDotS;
    }
    
    occlusion = 1.0 - (occlusion / ubo.kernelSize);
    //float finalOcclusion = pow(occlusion, power);
    return vec3(occlusion);
}