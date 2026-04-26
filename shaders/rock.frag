#version 460 core

in vec3 vNormal;
in vec3 vFragPos;
in vec4 vLightSpacePos;

out vec4 FragColor;

uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform float uAmbient;
uniform bool uShadowsEnabled;

layout(binding = 0) uniform sampler2D uShadowMap;

float shadowFactor(float NdotL) {
    vec3 proj = vLightSpacePos.xyz / vLightSpacePos.w;
    proj = proj * 0.5 + 0.5;

    if (proj.z > 1.0 || proj.x < 0.0 || proj.x > 1.0 || proj.y < 0.0 || proj.y > 1.0)
        return 0.0;

    float bias = mix(0.005, 0.0002, NdotL);
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
    vec3 n = normalize(vNormal);
    float NdotL = dot(n, uLightDir);
    float diff = max(NdotL, 0.0);

    float shadow = 0.0;
    if (uShadowsEnabled && NdotL > 0.0)
        shadow = shadowFactor(NdotL);

    vec3 rockColor = vec3(0.45, 0.43, 0.40);
    vec3 lighting = uLightColor * diff * (1.0 - shadow * 0.7) + uAmbient;
    FragColor = vec4(rockColor * lighting, 1.0);
}