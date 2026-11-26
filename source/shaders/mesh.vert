#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(set = 1, binding = 0) uniform VertexUniforms {
    mat4 mvp;
    mat4 model;
} uVert;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragUV;

void main() {
    gl_Position = uVert.mvp * vec4(inPos, 1.0);
    fragNormal = mat3(uVert.model) * inNormal;
    fragUV = inUV;
}