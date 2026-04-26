#pragma once
#include <glm/glm.hpp>
#include <vector>

inline float N(int i, int p, float t, const std::vector<float>& k) {
    if (p == 0) return (t >= k[i] && t < k[i + 1]) ? 1.f : 0.f;
    float d1 = k[i + p] - k[i];
    float d2 = k[i + p + 1] - k[i + 1];
    float r = 0.f;
    if (d1 > 1e-6f) r += (t - k[i]) / d1 * N(i, p - 1, t, k);
    if (d2 > 1e-6f) r += (k[i + p + 1] - t) / d2 * N(i + 1, p - 1, t, k);
    return r;
}

inline glm::vec3 evaluateNURBS(const std::vector<glm::vec3>& pts,
    const std::vector<float>& weights,
    const std::vector<float>& k, int deg, float t) {
    t = glm::clamp(t, k.front(), k.back() - 1e-5f);
    glm::vec3 num(0);
    float den = 0.f;
    for (int i = 0; i < static_cast<int>(pts.size()); i++) {
        float b = N(i, deg, t, k) * weights[i];
        num += b * pts[i];
        den += b;
    }
    return den > 1e-6f ? num / den : pts[0];
}

inline std::vector<float> generateClampedKnots(int n, int deg) {
    int nk = n + deg + 1;
    std::vector k(nk, 0.f);
    int intern = nk - 2 * (deg + 1);
    for (int i = 1; i <= intern; i++) k[i + deg] = static_cast<float>(i);
    for (int i = nk - deg - 1; i < nk; i++) k[i] = static_cast<float>(intern + 1);
    return k;
}