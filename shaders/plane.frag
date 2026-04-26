#version 460 core

in vec3 vFragPos;
in vec4 vLightSpacePos;

out vec4 FragColor;

uniform float uAmbient;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform bool uShadowsEnabled;

layout(binding = 0) uniform sampler2D uShadowMap;

float shadowFactor() {
    vec3 proj = vLightSpacePos.xyz / vLightSpacePos.w;
    proj = proj * 0.5 + 0.5;

    if (proj.z > 1.0 || proj.x < 0.0 || proj.x > 1.0 || proj.y < 0.0 || proj.y > 1.0)
        return 0.0;

    float bias = 0.001;
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(uShadowMap, 0);

    for (int x = -1; x <= 1; x++)
        for (int y = -1; y <= 1; y++) {
            float depth = texture(uShadowMap, proj.xy + vec2(x, y) * texelSize).r;
            shadow += (proj.z - bias) > depth ? 1.0 : 0.0;
        }

    return shadow / 9.0;
}

void main() {
    vec3 col = vec3(0.416, 0.77, 0.275);
    float diff = max(dot(vec3(0.0, 1.0, 0.0), uLightDir), 0.0);
    float shadow = uShadowsEnabled ? shadowFactor() : 0.0;

    vec3 lighting = uLightColor * diff * (1.0 - shadow * 0.7) + uAmbient;
    FragColor = vec4(col * lighting, 1.0);
}