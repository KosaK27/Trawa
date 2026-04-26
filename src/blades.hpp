#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <vector>
#include <random>
#include <cmath>
#include "wind.hpp"

inline std::mt19937& rng() {
    static std::mt19937 gen(std::random_device{}());
    return gen;
}

inline float randF() {
    static std::uniform_real_distribution dist(0.f, 1.f);
    return dist(rng());
}

struct Blade {
    std::vector<glm::vec3> ctrl, anim;
    std::vector<float> weights;
    float width = 0.f;
    float height = 0.f;
    float radialAngle = 0.f;
    float ph[3] = {};
};

inline Blade createBlade(float cx, float cz) {
    Blade b;
    b.height = 0.12f + randF() * 0.16f;
    b.width = 0.004f + randF() * 0.005f;
    b.radialAngle = randF() * glm::two_pi<float>();

    for (auto& p : b.ph) p = randF() * glm::two_pi<float>();

    float tilt = glm::radians(randF() * 20.f);
    glm::vec3 base(cx, -0.01f, cz);
    glm::vec3 tip(
        cx + b.height * std::sin(tilt) * std::cos(b.radialAngle),
        b.height * std::cos(tilt),
        cz + b.height * std::sin(tilt) * std::sin(b.radialAngle)
    );
    glm::vec3 bow(
        std::cos(b.radialAngle) * b.height * 0.04f,
        0.f,
        std::sin(b.radialAngle) * b.height * 0.04f
    );

    b.weights = { 1.5f, 1.3f, 1.f, 0.8f, 0.6f };
    b.ctrl = {
        base,
        glm::mix(base, tip, 0.25f) + bow * 0.5f,
        glm::mix(base, tip, 0.50f) + bow,
        glm::mix(base, tip, 0.75f) + bow * 0.7f,
        tip
    };
    b.anim = b.ctrl;
    return b;
}

inline void updateBlade(Blade& b, float t, const WindParams& w) {
    b.anim = b.ctrl;
    float dx = std::cos(w.direction);
    float dz = std::sin(w.direction);
    int n = static_cast<int>(b.anim.size());

    for (int i = 1; i < n; i++) {
        float f = static_cast<float>(i) / static_cast<float>(n - 1);
        f *= f;

        float sway =
            0.55f * std::sin(w.frequency * t + b.ph[0]) +
            0.30f * std::sin(w.frequency * 2.5f * t + b.ph[1]) * w.gustiness +
            0.15f * std::sin(w.frequency * 5.f * t + b.ph[2]) * w.gustiness;

        float flutter = 0.20f * std::sin(w.frequency * 2.f * t + b.ph[1]) * w.gustiness;

        b.anim[i].x += w.swayAmount * f * (sway * dx - flutter * dz);
        b.anim[i].z += w.swayAmount * f * (sway * dz + flutter * dx);
    }
}