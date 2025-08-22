#version 460

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 VP;          // ViewProjection
} ubo;

// No vertex inputs at all
layout(push_constant) uniform LinePC {
    vec4 A;           // world-space A.xyz (w ignored)
    vec4 B;           // world-space B.xyz (w ignored)
    vec4 Color;       // rgba 0..1
} pc;

layout(location = 0) out vec4 vColor;

void main() {
    vec3 P = (gl_VertexIndex == 0) ? pc.A.xyz : pc.B.xyz;
    gl_Position = ubo.VP * vec4(P, 1.0);
    vColor = pc.Color;
}