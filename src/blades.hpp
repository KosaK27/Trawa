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

inline float hash(int n) {
    n = (n + 17) * 23;
    n = n ^ (n >> 13);
    n = n * 101 + 71;
    n = n ^ (n >> 7);
    return static_cast<float>(n % 101) / 50.f - 1.f;
}

inline float cubic(float a, float b, float c, float d, float t) {
    float p = (d - c) - (a - b);
    float q = (a - b) - p;
    float r = c - a;
    return p * t * t * t + q * t * t + r * t + b;
}

inline float valueNoise(float t, float ph) {
    t += ph * 13.7f;
    int i = static_cast<int>(std::floor(t));
    float f = t - static_cast<float>(i);
    return cubic(hash(i - 1), hash(i), hash(i + 1), hash(i + 2), f);
}

inline float gustEnvelope(float t, float ph) {
    return 0.5f + 0.5f * valueNoise(t * 0.13f, ph + 3.f);
}

struct Blade {
    std::vector<glm::vec3> ctrl, anim;
    std::vector<float> weights;
    float width = 0.f;
    float height = 0.f;
    float radialAngle = 0.f;
    float ph[4] = {};
    float spatialOff = 0.f;
};

inline Blade createBlade(float cx, float cz) {
    Blade b;
    b.height = 0.12f + randF() * 0.16f;
    b.width = 0.004f + randF() * 0.005f;
    b.radialAngle = randF() * glm::two_pi<float>();

    for (auto& p : b.ph) p = randF() * glm::two_pi<float>();

    float tilt = glm::radians(randF() * 25.f);
    glm::vec3 base(cx, -0.01f, cz);
    glm::vec3 tip(
        cx + b.height * std::sin(tilt) * std::cos(b.radialAngle),
        b.height * std::cos(tilt),
        cz + b.height * std::sin(tilt) * std::sin(b.radialAngle)
    );

    float bowMag = b.height * (0.10f + randF() * 0.08f);
    glm::vec3 bow(
        std::cos(b.radialAngle) * bowMag,
        0.f,
        std::sin(b.radialAngle) * bowMag
    );

    b.weights = { 0.5f, 0.8f, 1.0f, 1.5f, 2.0f };
    b.ctrl = {
        base,
        glm::mix(base, tip, 0.25f) + bow * 0.3f,
        glm::mix(base, tip, 0.50f) + bow,
        glm::mix(base, tip, 0.75f) + bow * 0.9f,
        tip
    };
    b.anim = b.ctrl;
    return b;
}

inline void updateBlade(Blade& b, float t, const WindParams& w) {
    b.anim = b.ctrl;

    float dir = w.directionRad();
    float dx = std::cos(dir);
    float dz = std::sin(dir);
    float spatialT = t - b.spatialOff;
    float gust = gustEnvelope(spatialT, b.ph[0]);

    float rawSway =
        0.55f * valueNoise(w.frequency * spatialT * 0.7f, b.ph[0]) +
        0.30f * valueNoise(w.frequency * spatialT * 1.9f, b.ph[1]) * w.gustiness +
        0.15f * valueNoise(w.frequency * spatialT * 4.3f, b.ph[2]) * w.gustiness;

    float sway = rawSway > 0.f ? rawSway * gust : rawSway * gust * 0.35f;

    float bias = w.swayAmount * 0.6f;

    int n = static_cast<int>(b.anim.size());
    for (int i = 1; i < n; i++) {
        float fi = static_cast<float>(i) / static_cast<float>(n - 1);
        float bend = glm::smoothstep(0.0f, 1.0f, (fi - 0.15f) / 0.85f);
        bend = bend * bend * bend;

        float phaseShift = fi * 1.1f;
        float localRaw =
            0.55f * valueNoise(w.frequency * (spatialT + phaseShift) * 0.7f, b.ph[0]) +
            0.30f * valueNoise(w.frequency * (spatialT + phaseShift) * 1.9f, b.ph[1]) * w.gustiness +
            0.15f * valueNoise(w.frequency * (spatialT + phaseShift) * 4.3f, b.ph[2]) * w.gustiness;

        float localSway = localRaw > 0.f ? localRaw * gust : localRaw * gust * 0.35f;

        float localFlutter =
            w.gustiness * 0.45f *
            valueNoise(w.frequency * (spatialT + phaseShift) * 3.1f, b.ph[3] + 2.7f) *
            gust;

        b.anim[i].x += w.swayAmount * bend * (localSway * dx - localFlutter * dz) + bias * bend * dx;
        b.anim[i].z += w.swayAmount * bend * (localSway * dz + localFlutter * dx) + bias * bend * dz;

        if (i == n - 1)
            b.anim[i].y -= w.swayAmount * bend * std::abs(localSway) * 0.35f;
    }
}