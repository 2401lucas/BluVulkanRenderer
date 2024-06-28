#ifndef SSAO
    #define SSAO 1
#endif

vec3 ApplySSAO(vec3 col) {
    vec3 origin = vViewRay * texture(uTexLinearDepth, vTexcoord).r;
    vec3 normal = texture(uTexNormals, vTexcoord).xyz * 2.0 - 1.0;
    normal = normalize(normal);

    vec3 rvec = texture(uTexRandom, vTexcoord * uNoiseScale).xyz * 2.0 - 1.0;
    vec3 tangent = normalize(rvec - normal * dot(rvec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 tbn = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    for (int i = 0; i < uSampleKernelSize; ++i) {
    // get sample position:
       vec3 sample = tbn * uSampleKernel[i];
       sample = sample * uRadius + origin;
  
    // project sample position:
       vec4 offset = vec4(sample, 1.0);
       offset = uProjectionMat * offset;
       offset.xy /= offset.w;
       offset.xy = offset.xy * 0.5 + 0.5;
  
    // get sample depth:
       float sampleDepth = texture(uTexLinearDepth, offset.xy).r;
  
    // range check & accumulate:
       float rangeCheck= abs(origin.z - sampleDepth) < uRadius ? 1.0 : 0.0;
       occlusion += (sampleDepth <= sample.z ? 1.0 : 0.0) * rangeCheck;
    }
    occlusion = 1.0 - (occlusion / uSampleKernelSize);
}



