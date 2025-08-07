#version 450

#extension GL_GOOGLE_include_directive : require
#include "input_structures.glsl"

layout (location = 0) in vec3 vDirection;

layout (location = 0) out vec4 outFragColor;

void main()
{
    outFragColor = vec4(vDirection, 1.0);
}
