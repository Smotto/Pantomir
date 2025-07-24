#version 450

#extension GL_GOOGLE_include_directive : require
#include "input_structures.glsl"

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

void main()
{
    float lightValue = max(dot(inNormal, sceneData.sunlightDirection.xyz), 0.1f);

    vec4 texColor = texture(colorTex, inUV);
    vec4 texEmissive = texture(emissiveTex, inUV);

    // Alpha masking (if enabled)
    if (materialData.alphaMode == 1 && texColor.a < materialData.alphaCutoff)
        discard;

    // Emissive
    vec3 emissive = texEmissive.rgb * materialData.emissiveFactors;
    vec3 color = inColor * texColor.rgb;
    vec3 ambient = color *  sceneData.ambientColor.xyz;

    vec3 litColor = color * lightValue * sceneData.sunlightColor.w + ambient + emissive;

    outFragColor = vec4(litColor, texColor.a);
}