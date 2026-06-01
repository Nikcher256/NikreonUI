#version 450

layout(location = 0) in vec2 inLocalPosition;
layout(location = 1) flat in vec2 inRectSize;
layout(location = 2) flat in vec4 inFillColor;
layout(location = 3) flat in vec4 inBorderColor;
layout(location = 4) flat in float inRadius;
layout(location = 5) flat in float inBorderWidth;

layout(location = 0) out vec4 outColor;

float roundedRectDistance(vec2 point, vec2 halfSize, float radius)
{
    vec2 q = abs(point) - halfSize + vec2(radius);
    return length(max(q, vec2(0.0))) + min(max(q.x, q.y), 0.0) - radius;
}

void main()
{
    float radius = clamp(inRadius, 0.0, min(inRectSize.x, inRectSize.y) * 0.5);
    float borderWidth = max(inBorderWidth, 0.0);
    vec2 centered = inLocalPosition - inRectSize * 0.5;
    float distance = roundedRectDistance(centered, inRectSize * 0.5, radius);
    float antialias = max(fwidth(distance), 0.0001);

    float outerAlpha = 1.0 - smoothstep(0.0, antialias, distance);
    float fillAmount = borderWidth <= 0.0
        ? 1.0
        : 1.0 - smoothstep(0.0, antialias, distance + borderWidth);

    vec4 color = mix(inBorderColor, inFillColor, fillAmount);
    color.a *= outerAlpha;

    if (color.a <= 0.001) {
        discard;
    }

    outColor = color;
}
