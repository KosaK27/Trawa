#version 460 core

layout(location = 0) in vec3 aPos;

uniform mat4 uMVP;
uniform mat4 uLightMVP;

out vec3 vFragPos;
out vec4 vLightSpacePos;

void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vFragPos = aPos;
    vLightSpacePos = uLightMVP * vec4(aPos, 1.0);
}