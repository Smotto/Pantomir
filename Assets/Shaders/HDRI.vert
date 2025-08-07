#version 450

layout (location = 0) out vec3 vDirection;

layout(push_constant) uniform PushConstants {
    mat4 viewMatrix;
    mat4 projectionMatrix;
} pushConstants;

vec3 positions[3] = vec3[](
    vec3(-1.0, -1.0, 1.0),
    vec3(3.0, -1.0, 1.0),
    vec3(-1.0, 3.0, 1.0)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 1.0);
    // This builds a fullscreen triangle and avoids a vertex buffer entirely.

    // In fragment shader, we’ll use gl_FragCoord to reconstruct direction
    // OR you can do a projection here if rendering to each cubemap face separately
    vDirection = (inverse(pushConstants.projectionMatrix * pushConstants.viewMatrix) * gl_Position).xyz;
}
