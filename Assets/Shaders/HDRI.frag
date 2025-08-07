#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outFragColor;

layout(set = 0, binding = 0) uniform sampler2D hdrImage;

void main() {
    outFragColor = texture(hdrImage, vUV);
}
