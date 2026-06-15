#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <vector>
#include <cmath>
#include "nurbs.hpp"
#include "blades.hpp"

struct Vert { glm::vec3 pos, col, norm; };

inline GLuint makeVAO(GLuint vbo, bool raw = false) {
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    if (raw) {
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
    } else {
        GLsizei s = sizeof(Vert);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, s, reinterpret_cast<void*>(offsetof(Vert, pos)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, s, reinterpret_cast<void*>(offsetof(Vert, col)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, s, reinterpret_cast<void*>(offsetof(Vert, norm)));
    }
    glBindVertexArray(0);
    return vao;
}

template<typename T>
inline void upload(GLuint vbo, const std::vector<T>& v, GLenum u = GL_DYNAMIC_DRAW) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(v.size() * sizeof(T)), v.data(), u);
}

static constexpr int BLADE_STEPS = 32;

inline void buildBlade(const Blade& b, const std::vector<float>& knots, int deg, std::vector<Vert>& out) {
    std::vector<glm::vec3> pts(BLADE_STEPS + 1);
    float t0 = knots.front(), t1 = knots.back();

    for (int i = 0; i <= BLADE_STEPS; i++)
        pts[i] = evaluateNURBS(b.anim, b.weights, knots, deg,
            t0 + (t1 - t0) * static_cast<float>(i) / static_cast<float>(BLADE_STEPS));

    glm::vec3 side = glm::normalize(glm::vec3(-std::sin(b.radialAngle), 0.f, std::cos(b.radialAngle)));
    glm::vec3 col(0.13f, 0.45f, 0.08f);

    for (int i = 0; i < BLADE_STEPS; i++) {
        float u0 = static_cast<float>(i) / static_cast<float>(BLADE_STEPS);
        float u1 = static_cast<float>(i + 1) / static_cast<float>(BLADE_STEPS);
        float w0 = b.width * std::pow(1.f - u0, 3.f);
        float w1 = b.width * std::pow(1.f - u1, 3.f);

        glm::vec3 tang = glm::normalize(pts[i + 1] - pts[i] + glm::vec3(1e-6f));
        glm::vec3 norm = glm::normalize(glm::cross(side, tang));
        glm::vec3 L0 = pts[i] - side * w0, R0 = pts[i] + side * w0;
        glm::vec3 L1 = pts[i + 1] - side * w1, R1 = pts[i + 1] + side * w1;

        out.push_back({ L0, col, norm });
        out.push_back({ R0, col, norm });
        out.push_back({ L1, col, norm });
        out.push_back({ L1, col, norm });
        out.push_back({ R0, col, norm });
        out.push_back({ R1, col, norm });
    }
}

inline std::vector<Vert> buildPlane(float s = 0.6f) {
    glm::vec3 n(0.f, 1.f, 0.f), c(0.25f, 0.65f, 0.25f);
    return {
        {{-s,0,-s},c,n}, {{-s,0,s},c,n}, {{s,0,s},c,n},
        {{-s,0,-s},c,n}, {{s,0,s},c,n},  {{s,0,-s},c,n}
    };
}

inline std::vector<glm::vec3> buildSphere(float r = 0.025f, int st = 16, int sl = 16) {
    std::vector<glm::vec3> v;
    for (int i = 0; i < st; i++) {
        float p0 = glm::pi<float>() * (-0.5f + static_cast<float>(i) / static_cast<float>(st));
        float p1 = glm::pi<float>() * (-0.5f + static_cast<float>(i + 1) / static_cast<float>(st));
        for (int j = 0; j < sl; j++) {
            float t0 = 2.f * glm::pi<float>() * static_cast<float>(j) / static_cast<float>(sl);
            float t1 = 2.f * glm::pi<float>() * static_cast<float>(j + 1) / static_cast<float>(sl);
            glm::vec3 v00(r*std::cos(p0)*std::cos(t0), r*std::sin(p0), r*std::cos(p0)*std::sin(t0));
            glm::vec3 v10(r*std::cos(p1)*std::cos(t0), r*std::sin(p1), r*std::cos(p1)*std::sin(t0));
            glm::vec3 v01(r*std::cos(p0)*std::cos(t1), r*std::sin(p0), r*std::cos(p0)*std::sin(t1));
            glm::vec3 v11(r*std::cos(p1)*std::cos(t1), r*std::sin(p1), r*std::cos(p1)*std::sin(t1));
            v.push_back(v00); v.push_back(v10); v.push_back(v11);
            v.push_back(v00); v.push_back(v11); v.push_back(v01);
        }
    }
    return v;
}

struct GrassBunch {
    std::vector<Blade> blades;
    GLuint vbo = 0;
    GLuint vao = 0;
    int vertCount = 0;
};

inline void rebuildSpatialOffsets(GrassBunch& g, const WindParams& w) {
    float dir = w.directionRad();
    float dx = std::cos(dir);
    float dz = std::sin(dir);
    for (auto& b : g.blades)
        b.spatialOff = (b.ctrl[0].x * dx + b.ctrl[0].z * dz) * 2.5f;
}

inline GrassBunch createGrassBunch(float cx, float cz, int count, float minLen, float maxLen) {
    GrassBunch g;
    g.blades.resize(count);
    for (auto& b : g.blades) {
        b = createBlade(cx, cz);
        float t = randF();
        b.height = minLen + t * (maxLen - minLen);
        glm::vec3 base = b.ctrl[0], tip = b.ctrl[4];
        float origLen = glm::length(tip - base);
        if (origLen > 1e-6f)
            for (auto& p : b.ctrl) p = base + (p - base) * (b.height / origLen);
        b.anim = b.ctrl;
    }
    g.vertCount = static_cast<int>(g.blades.size()) * BLADE_STEPS * 6;
    glGenBuffers(1, &g.vbo);
    g.vao = makeVAO(g.vbo);
    return g;
}

inline std::vector<GrassBunch> createGrassBunches(int count) {
    std::vector<GrassBunch> bunches;
    bunches.reserve(count);
    for (int i = 0; i < count; i++) {
        float cx = randF() - 0.5f;
        float cz = randF() - 0.5f;
        int blades = 8 + static_cast<int>(randF() * 20.f);
        float minLen = 0.04f + randF() * 0.06f;
        float maxLen = minLen + 0.08f + randF() * 0.14f;
        bunches.push_back(createGrassBunch(cx, cz, blades, minLen, maxLen));
    }
    return bunches;
}

struct Rock {
    glm::vec3 pos{};
    GLuint vbo = 0;
    GLuint vao = 0;
    int vertCount = 0;
};

inline std::vector<Rock> createRocks(int count, float radius) {
    auto sv = buildSphere(radius);
    std::vector<Rock> rocks;
    rocks.reserve(count);
    for (int i = 0; i < count; i++) {
        Rock r;
        r.pos = glm::vec3(randF() - 0.5f, -radius * 0.5f, randF() - 0.5f);
        r.vertCount = static_cast<int>(sv.size());
        glGenBuffers(1, &r.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, r.vbo);
        glBufferData(GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(sv.size() * sizeof(glm::vec3)),
            sv.data(), GL_STATIC_DRAW);
        r.vao = makeVAO(r.vbo, true);
        rocks.push_back(r);
    }
    return rocks;
}