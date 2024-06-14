#version 450

layout(location = 0) in vec2 fragTexCord;

layout (binding = 0) uniform UBO 
{
	bool useFXAA;
    float fxaaEdgeThresholdMin;
    float fxaaEdgeThresholdMax;
    float pixelBlend;
} ubo;

layout(binding = 1) uniform sampler2D screenTexture;

layout (location = 0) out vec4 outColor;

// Estimates Luminances from RG
float fxaaLuma(vec3 rgb) {
    return rgb.y * (0.587/0.299) + rgb.x;
    //return 0.299*rgb.x + 0.587*rgb.y + 0.114*rgb.z;
    //return 0.2126*rgb.x + 0.7152*rgb.y + 0.0722*rgb.z;
}

// Sample Main & surrounding pixels, compare luminance and average 
void main() {
    vec3 colorCenter =  texture(screenTexture, fragTexCord).rgb;
    if(!ubo.useFXAA){
        outColor = vec4(colorCenter, 1.0f);
        return;
    }

    vec3 rgbN =         textureOffset(screenTexture, fragTexCord, ivec2(0,1)).rgb;
    vec3 rgbS =         textureOffset(screenTexture, fragTexCord, ivec2(0,-1)).rgb;
    vec3 rgbE =         textureOffset(screenTexture, fragTexCord, ivec2(1,0)).rgb;
    vec3 rgbW =         textureOffset(screenTexture, fragTexCord, ivec2(-1,0)).rgb;

    float lumaC = fxaaLuma(colorCenter);
    float lumaN = fxaaLuma(rgbN);
    float lumaS = fxaaLuma(rgbS);
    float lumaE = fxaaLuma(rgbE);
    float lumaW = fxaaLuma(rgbW);

    float lumaMin = min(lumaC, min(min(lumaS, lumaN), min(lumaE, lumaW)));
    float lumaMax = max(lumaC, max(max(lumaS, lumaN), max(lumaE, lumaW)));

    float lumaRange = lumaMax - lumaMin;

    //If variation is lower or in dark area, Skip AA (combine with other shaders)
    if(lumaRange < max(ubo.fxaaEdgeThresholdMin, lumaMax * ubo.fxaaEdgeThresholdMax)) {
        outColor = vec4(colorCenter, 1.0f);
        return;
    }

    // Calculate Blend amount
    vec3 rgbNW = textureOffset(screenTexture, fragTexCord, ivec2(-1,-1)).rgb;
    vec3 rgbNE = textureOffset(screenTexture, fragTexCord, ivec2(1,-1)).rgb;
    vec3 rgbSW = textureOffset(screenTexture, fragTexCord, ivec2(-1,1)).rgb;
    vec3 rgbSE = textureOffset(screenTexture, fragTexCord, ivec2(1,1)).rgb;
    
    float lumaNW = fxaaLuma(rgbNW);
    float lumaNE = fxaaLuma(rgbNE);
    float lumaSE = fxaaLuma(rgbSE);
    float lumaSW = fxaaLuma(rgbSW);

    // Double immidiate neighbour weight as the pixels are closer to our target pixel are more important
    float hFilter = 2 * (lumaN + lumaS + lumaE + lumaW);
    hFilter += lumaNW + lumaNE + lumaSW + lumaSE;
	hFilter *= 1.0 / 12; //Div by 12 because we sample NESW neighbours twice
    //Smooth filter
    float blendFactor = smoothstep(0, 1,clamp(hFilter - lumaC, 0.0, 1.0));
    blendFactor *= blendFactor;

	float horizontal =
	    abs(lumaN + lumaS - 2 * lumaC) * 2 +
		abs(lumaNE + lumaSE - 2 * lumaE) +
		abs(lumaNW + lumaSW - 2 * lumaW);
	float vertical =
		abs(lumaE + lumaW - 2 * lumaC) * 2 +
		abs(lumaNE + lumaNW - 2 * lumaN) +
		abs(lumaSE + lumaSW - 2 * lumaS);
	bool isHorizontal = horizontal >= vertical;

    float pGradient  = abs((isHorizontal ? lumaN : lumaE) - lumaC);
	float nGradient  = abs((isHorizontal ? lumaS : lumaW) - lumaC);


    vec2 texSize = textureSize(screenTexture, 0);
    float pixelStep = (1.0 / (isHorizontal ? texSize.y : texSize.x)) * ((pGradient < nGradient) ? -1.0 : 1.0);
    
    vec2 blurFragTexCord = fragTexCord;
    if (isHorizontal) {
		blurFragTexCord.y += pixelStep * ubo.pixelBlend;
	}
	else {
		blurFragTexCord.x += pixelStep * ubo.pixelBlend;
	}
	
    outColor = vec4(texture(screenTexture, blurFragTexCord).rgb, 1.0);
}