#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragUV;

layout(set = 2, binding = 0) uniform sampler2D texSampler;

layout(set = 3, binding = 0) uniform FragmentUniforms {
    vec3 lightDir;
    float ambient;
    vec2 _pad;
} uFrag;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 n = normalize(fragNormal);
    vec3 l = normalize(uFrag.lightDir);

    float ndotl = max(dot(n, l), 0.0);
    float brightness = clamp(uFrag.ambient + (1.0 - uFrag.ambient) * ndotl, 0.0, 1.0);

    vec4 texColor = texture(texSampler, fragUV);
    outColor = vec4(texColor.rgb * brightness, texColor.a);
}