//#version 450
//
//layout(binding = 1) uniform sampler2D texSampler;
//
//layout(location = 0) in vec3 fragColor;
//layout(location = 1) in vec2 fragTexCoord;
//
//layout(location = 0) out vec4 outColor;
//
//void main() {
//    outColor = texture(texSampler, fragTexCoord);
//}

#version 450

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 fragColor;  // Add color input

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPos;
    vec3 lightColor;
    vec3 viewPos;
    bool useBakedLighting;
} ubo;

layout(binding = 1) uniform sampler2D texSampler;

void main() {
    vec3 ambientMaterial = vec3(0.1, 0.1, 0.1);
    vec3 diffuseMaterial = vec3(1.0, 1.0, 1.0); // Hardcode white for models with no texture
    vec3 specularMaterial = vec3(0.5, 0.5, 0.5);
    float shininess = 32.0;

    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(ubo.lightPos - fragPos);
    vec3 viewDir = normalize(ubo.viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);

    vec3 ambient = ambientMaterial * ubo.lightColor;
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * diffuseMaterial * ubo.lightColor;
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = spec * specularMaterial * ubo.lightColor;

    vec3 result = ambient + diffuse + specular;
    outColor = texture(texSampler, fragTexCoord);
}