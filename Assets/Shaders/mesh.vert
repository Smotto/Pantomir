#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require

#include "input_structures.glsl"

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec2 outUV;
layout (location = 3) out vec3 outTangent;
layout (location = 4) out vec3 outBitangent;
layout (location = 5) out vec3 outNormalWS;
layout (location = 6) out vec3 outWorldPos;

struct Vertex {
    vec3 position;
    float uv_x;
    vec3 normal;
    float uv_y;
    vec4 tangent;
    vec4 color;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer {
    Vertex vertices[];
};

layout(push_constant) uniform constants {
    mat4 renderMatrix;
    VertexBuffer vertexBuffer;
} PushConstants;

void main()
{
    Vertex vertex = PushConstants.vertexBuffer.vertices[gl_VertexIndex];

    vec3 position = vertex.position;
    vec3 normal = vertex.normal;
    vec3 tangent = vertex.tangent.xyz;
    float handedness = vertex.tangent.w;
    vec3 bitangent = cross(normal, tangent) * handedness;

    vec2 uv = vec2(vertex.uv_x, vertex.uv_y);
    vec3 color = vertex.color.rgb;

    vec4 worldPos = PushConstants.renderMatrix * vec4(position, 1.0);
    gl_Position = sceneData.viewproj * worldPos;

    mat3 normalMatrix = transpose(inverse(mat3(PushConstants.renderMatrix)));

    outNormal     = normalize(normal);
    outNormalWS   = normalize(normalMatrix * normal);
    outTangent    = normalize(normalMatrix * tangent);
    outBitangent  = normalize(normalMatrix * bitangent);
    outUV         = uv;
    outColor      = color;
    outWorldPos   = worldPos.xyz;
}
