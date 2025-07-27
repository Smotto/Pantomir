#version 450

#extension GL_GOOGLE_include_directive : require
#include "input_structures.glsl"

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

void main()
{
    vec3 normal = normalize(inNormal);
    vec3 lightDir = normalize(-sceneData.sunlightDirection.xyz); // Note the negative sign - light direction should point FROM light TO surface

    // More forgiving lighting calculation
    float NdotL = max(dot(normal, lightDir), 0.0);
    float lightValue = NdotL * 0.5 + 0.5; // Remap for half-lambert
    lightValue = max(lightValue, 0.3); // Ensure minimum lighting

    vec4 texColor = texture(colorTex, inUV);
    vec4 texMetalRoughness = texture(metalRoughTex, inUV);
    vec4 texEmissive = texture(emissiveTex, inUV);
    vec4 texNormal = texture(normalTex, inUV);

    // Alpha masking (if enabled)
    if (materialData.alphaMode == 1 && texColor.a < materialData.alphaCutoff)
        discard;

    vec3 albedo = inColor * texColor.rgb;
    float roughness = texMetalRoughness.g;
    float metallic = texMetalRoughness.b;
    vec3 emissive = texEmissive.rgb * materialData.emissiveFactors;

    // Simple metallic workflow
    vec3 baseReflectivity = mix(vec3(0.04), albedo, metallic);
    vec3 diffuseColor = albedo * (1.0 - metallic);

    // Basic lighting with proper energy conservation
    vec3 ambient = diffuseColor * sceneData.ambientColor.xyz * 0.3; // Boost ambient
    vec3 diffuse = diffuseColor * lightValue * sceneData.sunlightColor.xyz * sceneData.sunlightColor.w;

    // More robust specular calculation
    vec3 viewDir = normalize(vec3(0.0, 0.0, 1.0)); // Simple view direction approximation
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float NdotH = max(dot(normal, halfwayDir), 0.0);
    float NdotV = max(dot(normal, viewDir), 0.01); // Prevent division by zero

    // Clamp roughness to prevent artifacts
    float clampedRoughness = max(roughness, 0.04);
    float alpha = clampedRoughness * clampedRoughness;
    float alpha2 = alpha * alpha;

    // Simplified GGX distribution
    float denom = NdotH * NdotH * (alpha2 - 1.0) + 1.0;
    float D = alpha2 / (3.14159265 * denom * denom);

    // Simple geometry function
    float k = (clampedRoughness + 1.0) * (clampedRoughness + 1.0) / 8.0;
    float G1L = NdotL / (NdotL * (1.0 - k) + k);
    float G1V = NdotV / (NdotV * (1.0 - k) + k);
    float G = G1L * G1V;

    // Fresnel
    float VdotH = max(dot(viewDir, halfwayDir), 0.0);
    vec3 F = baseReflectivity + (1.0 - baseReflectivity) * pow(clamp(1.0 - VdotH, 0.0, 1.0), 5.0);

    // Final specular term with proper normalization and brightness boost
    vec3 numerator = D * G * F;
    float denominator = 4.0 * NdotV * NdotL + 0.001; // Prevent division by zero
    vec3 specular = numerator / denominator * 0.5; // Scale down specular to prevent over-brightness

    vec3 finalColor = ambient + diffuse + specular * NdotL * sceneData.sunlightColor.xyz * sceneData.sunlightColor.w * 2.0 + emissive;

    // Ensure minimum brightness to prevent pure black
    finalColor = max(finalColor, albedo * 0.1);

    // DEBUG: Uncomment these lines one at a time to debug
    // outFragColor = vec4(albedo, texColor.a); return; // Show base color
    // outFragColor = vec4(normal * 0.5 + 0.5, 1.0); return; // Show normals
    // outFragColor = vec4(vec3(lightValue), 1.0); return; // Show lighting
    // outFragColor = vec4(vec3(metallic), 1.0); return; // Show metallic values
    // outFragColor = vec4(vec3(roughness), 1.0); return; // Show roughness values

    // TEMPORARY: Much simpler lighting for debugging
    vec3 simpleColor = albedo * (lightValue * 0.8 + 0.4); // Simple shading
    outFragColor = vec4(simpleColor + emissive, texColor.a);
    return;

    outFragColor = vec4(finalColor, texColor.a);
}