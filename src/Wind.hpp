#pragma once

struct WindParams {
    float swayAmount = 0.05f;
    float frequency = 1.2f;
    float gustiness = 0.6f;
    float directionDeg = 0.0f;

    float directionRad() const {
        return directionDeg * 3.14159265f / 180.f;
    }
};