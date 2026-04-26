#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include <cmath>

struct Camera {
    float yaw = 30.f;
    float pitch = 20.f;
    float dist = 0.75f;
    glm::vec3 target{ 0.f, 0.08f, 0.f };
    bool drag = false;
    double lx = 0.0, ly = 0.0;

    glm::mat4 view() const {
        float p = glm::radians(pitch);
        float y = glm::radians(yaw);
        glm::vec3 eye = target + dist * glm::vec3(
            std::cos(p) * std::sin(y),
            std::sin(p),
            std::cos(p) * std::cos(y)
        );
        return glm::lookAt(eye, target, { 0.f, 1.f, 0.f });
    }

    void reset() { yaw = 30.f; pitch = 20.f; dist = 0.75f; }
};

inline Camera cam;

inline void onMouse(GLFWwindow*, int btn, int act, int) {
    if (ImGui::GetIO().WantCaptureMouse) return;
    if (btn == GLFW_MOUSE_BUTTON_LEFT) cam.drag = (act == GLFW_PRESS);
}

inline void onCursor(GLFWwindow*, double x, double y) {
    if (!ImGui::GetIO().WantCaptureMouse && cam.drag) {
        cam.yaw += static_cast<float>(x - cam.lx) * 0.4f;
        cam.pitch -= static_cast<float>(y - cam.ly) * 0.4f;
    }
    cam.lx = x;
    cam.ly = y;
}

inline void onScroll(GLFWwindow*, double, double dy) {
    if (!ImGui::GetIO().WantCaptureMouse)
        cam.dist = glm::clamp(cam.dist - static_cast<float>(dy) * 0.05f, 0.1f, 3.f);
}