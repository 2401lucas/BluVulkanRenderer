#version 450

#include "../includes/PostProcessing/postProcessingFragGeneric.glsl"

void main(){
    outColor = vec4(texture(screenTexture, fragTexCord).rgb, 0.0);
}