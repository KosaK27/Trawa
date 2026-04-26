#version 460 core

out vec4 FragColor;

uniform vec3 uSunColor;

void main() {
    FragColor = vec4(uSunColor, 1.0);
}