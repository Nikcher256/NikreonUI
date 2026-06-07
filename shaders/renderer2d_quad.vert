#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inUv;
layout(location = 3) in uint inTextureSlot;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec2 outUv;
layout(location = 2) flat out uint outTextureSlot;

void main()
{
    gl_Position = vec4(inPosition, 0.0, 1.0);
    outColor = inColor;
    outUv = inUv;
    outTextureSlot = inTextureSlot;
}