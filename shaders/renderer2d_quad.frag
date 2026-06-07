#version 450

layout(set = 0, binding = 0) uniform sampler2D uTexture;

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec2 inUv;
layout(location = 0) out vec4 outColor;

void main()
{
    outColor = inColor * texture(uTexture, inUv);
}
