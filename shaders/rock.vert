#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

uniform mat4 uMVP;
uniform mat4 uModel;
uniform mat4 uLightMVP;
uniform mat3 uNormalMatrix;

out vec3 vNormal;
out vec3 vFragPos;
out vec4 vLightSpacePos;

void main() {
    vNormal = normalize(uNormalMatrix * aNormal);
    vFragPos = vec3(uModel * vec4(aPos, 1.0));
    vLightSpacePos = uLightMVP * vec4(vFragPos, 1.0);
    gl_Position = uMVP * vec4(aPos, 1.0);
}