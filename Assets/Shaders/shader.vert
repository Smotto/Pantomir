#version 450

layout(push_constant) uniform PushConstants {
    uint modelIndex;
} pushConstants;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPos;
    vec3 lightColor;
    vec3 viewPos;
    mat4 modelMatrices[1];
} ubo;

layout(location = 0) in vec3 inPosition;  // Matches pos at location 0
layout(location = 1) in vec3 inColor;     // Matches color at location 1
layout(location = 2) in vec2 inTexCoord;  // Matches texCoord at location 2
layout(location = 3) in vec3 inNormal;    // Matches normal at location 3

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out vec3 fragColor;

void main() {
    mat4 model = ubo.modelMatrices[pushConstants.modelIndex]; // Select model matrix dynamically
    gl_Position = ubo.proj * ubo.view * model * vec4(inPosition, 1.0);
    fragPos = vec3(model * vec4(inPosition, 1.0));
    fragNormal = mat3(transpose(inverse(model))) * inNormal;
    fragTexCoord = inTexCoord;
    fragColor = inColor;
}