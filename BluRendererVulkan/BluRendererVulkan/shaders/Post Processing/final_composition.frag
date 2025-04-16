#version 450

layout(set = 0, binding = 0) uniform sampler2D textures[2];

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 fragColor;

void main() {
    vec4 sceneColor = texture(textures[0], vUV);
    vec4 uiColor    = texture(textures[1], vUV);

    // Basic alpha blending: UI over scene
    fragColor = mix(sceneColor, uiColor, uiColor.a);
}