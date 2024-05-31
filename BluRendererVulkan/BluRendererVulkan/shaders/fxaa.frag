#version 450

#define FXAA_PRESET 3


layout (location = 0) out vec4 outColor;

//Estimates Luminances from RG
float fxaaLuma(float3 rbg) {
    return rgb.y * (0.587/0.299) + rgb.x;
}

void main() {
    vec3 colorCenter = texture(sampler2D(screenTexture, sClampLinear), uv).rgb;

    float lumaC = fxaaLuma(colorCenter)
    float lumaN = fxaaLuma(rgbN);
    float lumaS = fxaaLuma(rgbN);
    float lumaE = fxaaLuma(rgbN);
    float lumaW = fxaaLuma(rgbN);

    float lumaMin = min(lumaC, min(min(lumaS, lumaN), min(lumaE, lumaW)));
    float lumaMax = max(lumaC, max(max(lumaS, lumaN), max(lumaE, lumaW)));

    float lumaRange = lumaMax - lumaMin;

    //If variation is lower or in dark area, Skip AA (combine with other shaders)
    if(lumaRange < max(EDGE_THRESHOLD_MIN, lumaMax * EDGE_THRESHOLD_MAX)) {
        outColor = vec4(colorCenter, 1.0f);
        return;
    }

    float lumaNE = fxaaLuma(textureLodOffset(sampler2D(screenTexture, sClampLinear), uv, 0.0, ivec2( 1, 1)).rgb);
    float lumaNW = fxaaLuma(textureLodOffset(sampler2D(screenTexture, sClampLinear), uv, 0.0, ivec2( 1,-1)).rgb);
    float lumaSE = fxaaLuma(textureLodOffset(sampler2D(screenTexture, sClampLinear), uv, 0.0, ivec2(-1, 1)).rgb);
    float lumaSW = fxaaLuma(textureLodOffset(sampler2D(screenTexture, sClampLinear), uv, 0.0, ivec2(-1,-1)).rgb);
}