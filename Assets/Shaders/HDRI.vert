#version 450

layout(push_constant) uniform PushConstants {
    mat4 viewMatrix;
    mat4 projectionMatrix;
} pushConstants;

layout(location = 0) out vec3 vDirection;

vec3 positions[3] = vec3[](
    vec3(-1.0, -1.0, 1.0),
    vec3(3.0, -1.0, 1.0),
    vec3(-1.0, 3.0, 1.0)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 1.0);

    // Transform NDC direction back to world space
    mat4 inverseViewProjection = inverse(pushConstants.projectionMatrix * pushConstants.viewMatrix);

    vec4 worldDir = inverseViewProjection * gl_Position;
    vDirection = worldDir.xyz / worldDir.w;
}
