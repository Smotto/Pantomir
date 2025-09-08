#version 450

#extension GL_GOOGLE_include_directive : require
#include "input_structures.glsl"

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inTangent;
layout (location = 4) in vec3 inBitangent;
layout (location = 5) in vec3 inNormalWS;
layout (location = 6) in vec3 inWorldPos;

layout (location = 0) out vec4 outFragColor;

void main()
{
    vec4 texColor       = texture(colorTexSampler, inUV);
    vec4 texMetalRough  = texture(metalRoughTexSampler, inUV);
    vec3 texEmissive    = texture(emissiveTexSampler, inUV).rgb;
    vec3 texSpecular    = texture(specularTexSampler, inUV).rgb;

    if (texColor.a < 0.1)
    discard;

    float roughness     = texMetalRough.g;
    float metallic      = texMetalRough.b;

    vec3 baseColor      = texColor.rgb;
    vec3 baseReflectivity = mix(vec3(0.04), baseColor, metallic);
    vec3 diffuseColor   = baseColor * (1.0 - metallic);

    vec3 normal         = normalize(inNormal);
    vec3 lightDir       = normalize(-sceneData.sunlightDirection.xyz);// FROM light to surface
    vec3 viewDir        = normalize(vec3(0.0, 0.0, 1.0));// camera facing forward (approx)
    vec3 halfwayDir     = normalize(lightDir + viewDir);

    float NdotL         = max(dot(normal, lightDir), 0.0);
    float NdotH         = max(dot(normal, halfwayDir), 0.0);
    float NdotV         = max(dot(normal, viewDir), 0.01);
    float VdotH         = max(dot(viewDir, halfwayDir), 0.0);

    float alpha         = roughness * roughness;
    float alpha2        = alpha * alpha;

    float denom         = NdotH * NdotH * (alpha2 - 1.0) + 1.0;
    float D             = alpha2 / (3.14159265 * denom * denom);

    float k             = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    float G1L           = NdotL / (NdotL * (1.0 - k) + k);
    float G1V           = NdotV / (NdotV * (1.0 - k) + k);
    float G             = G1L * G1V;

    vec3 F                = baseReflectivity + (1.0 - baseReflectivity) * pow(1.0 - VdotH, 5.0);
    vec3 specularStrength = texSpecular * materialData.specularFactor;
    vec3 specular         = D * G * F / (4.0 * NdotV * NdotL + 0.001);
//    specular *= specularStrength; // Not useful, pretty much destroys the values

    vec3 ambient        = diffuseColor * sceneData.ambientColor.rgb;
    vec3 diffuse        = diffuseColor * NdotL * sceneData.sunlightColor.rgb;
    vec3 finalSpecular  = specular * NdotL * sceneData.sunlightColor.rgb;
    vec3 emissive       = texEmissive * materialData.emissiveFactors * materialData.emissiveStrength;

    vec3 finalColor     = ambient + diffuse + finalSpecular + emissive;

    outFragColor = vec4(finalColor, texColor.a);
}
