#version 450

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inUV;

layout(set = 1, binding = 0) uniform UiUniforms {
    vec2 screenSize;
    vec2 _pad;
} uUi;

layout(location = 0) out vec2 fragUV;

void main() {
    vec2 ndc = (inPos / uUi.screenSize) * 2.0 - 1.0;
    ndc.y = -ndc.y; // top-left (0,0) in screen space
    gl_Position = vec4(ndc, 0.0, 1.0);
    fragUV = inUV;
}