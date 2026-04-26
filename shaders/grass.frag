#version 460 core

in vec3 vColor;
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

    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            float depth = texture(uShadowMap, proj.xy + vec2(x, y) * texelSize).r;
            shadow += (proj.z - bias) > depth ? 1.0 : 0.0;
        }
    }

    return shadow / 9.0;
}

void main() {
    vec3 n = normalize(vNormal);

    vec3 faceNormal = gl_FrontFacing
        ? normalize(mix(n, vec3(0.0, 1.0, 0.0), 0.7))
        : normalize(mix(-n, vec3(0.0, 1.0, 0.0), 0.7));

    float NdotL = dot(faceNormal, uLightDir);
    float diff = max(NdotL, 0.0);
    float rawNdotL = gl_FrontFacing ? dot(n, uLightDir) : dot(-n, uLightDir);

    float shadow = 0.0;
    if (uShadowsEnabled && rawNdotL > 0.0)
        shadow = shadowFactor(rawNdotL);

    vec3 lighting = uLightColor * diff * (1.0 - shadow * 0.7) + uAmbient;
    FragColor = vec4(vColor * lighting, 1.0);
}