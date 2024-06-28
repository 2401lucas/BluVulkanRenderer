#version 450

#include "../includes/PostProcessing/postProcessingFragGeneric.glsl"
#include "../includes/PostProcessing/fxaa.glsl"

void main(){
    int aliasingType = int(ubo.aaType);

    switch(aliasingType) {
    case 0:
        outColor = vec4(texture(screenTexture, fragTexCord).rgb, 0.0);
        break;
    case 1:
        outColor = vec4(applyFXAA(), 0.0);
        break;
    case 2:
        //outColor = vec4(applyTAA(), 0.0);
        break;
    }
}