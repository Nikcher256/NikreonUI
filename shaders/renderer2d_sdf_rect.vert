#version 450

layout(location = 0) in vec2 inQuadPosition;
layout(location = 1) in vec2 inQuadLocalPosition;
layout(location = 2) in vec2 inRectPosition;
layout(location = 3) in vec2 inRectNdcSize;
layout(location = 4) in vec2 inRectSize;
layout(location = 5) in vec4 inFillColor;
layout(location = 6) in vec4 inBorderColor;
layout(location = 7) in float inRadius;
layout(location = 8) in float inBorderWidth;

layout(location = 0) out vec2 outLocalPosition;
layout(location = 1) flat out vec2 outRectSize;
layout(location = 2) flat out vec4 outFillColor;
layout(location = 3) flat out vec4 outBorderColor;
layout(location = 4) flat out float outRadius;
layout(location = 5) flat out float outBorderWidth;

void main()
{
    vec2 position = inRectPosition + inQuadPosition * inRectNdcSize;

    gl_Position = vec4(position, 0.0, 1.0);
    outLocalPosition = inQuadLocalPosition * inRectSize;
    outRectSize = inRectSize;
    outFillColor = inFillColor;
    outBorderColor = inBorderColor;
    outRadius = inRadius;
    outBorderWidth = inBorderWidth;
}
