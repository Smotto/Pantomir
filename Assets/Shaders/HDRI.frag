#version 450

layout(location = 0) in vec3 vDirection;
layout(location = 0) out vec4 outFragColor;

layout(set = 0, binding = 0) uniform sampler2D hdrImage;

void main() {
    vec3 dir = normalize(vDirection);

    // No Y flip needed here
    vec2 uv;
    uv.x = atan(dir.z, dir.x) / (2.0 * 3.1415926) + 0.5;
    uv.y = asin(dir.y) / 3.1415926 + 0.5;

    vec3 hdrColor = texture(hdrImage, uv).rgb;

    // Tonemapping: Reinhard
    hdrColor = hdrColor / (hdrColor + vec3(1.0));

    // Gamma correction to sRGB
    hdrColor = pow(hdrColor, vec3(1.0 / 2.2));

    outFragColor = vec4(hdrColor, 1.0);
}