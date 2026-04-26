#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec3 aNormal;

uniform mat4 uMVP;
uniform mat4 uLightMVP;
uniform mat3 uNormalMatrix;

out vec3 vColor;
out vec3 vNormal;
out vec3 vFragPos;
out vec4 vLightSpacePos;

void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vColor = aColor;
    vNormal = normalize(uNormalMatrix * aNormal);
    vFragPos = aPos;
    vLightSpacePos = uLightMVP * vec4(aPos, 1.0);
}