#pragma once

#include <cmath>

namespace bo1::android::pad {

// Applied once to the vector magnitude; direction is preserved, including at
// square-gate corners. This is our own reverse-S curve, not a retail CoD curve.
inline void ShapeLook(float& pitch, float& yaw, int mode, float adsFraction = 0.0f)
{
    const float magnitude = std::sqrt(pitch * pitch + yaw * yaw);
    if (magnitude <= 0.0f) return;
    const float t = magnitude > 1.0f ? 1.0f : magnitude;
    const float dynamic = t + 0.5f * t * (1.0f - t) * (1.0f - 2.0f * t);
    // Keep the hip response, blend to a precision centre when ADS. Full-stick
    // speed remains unchanged; there is no smoothing latency or aim assistance.
    const float ads = adsFraction < 0.0f ? 0.0f : (adsFraction > 1.0f ? 1.0f : adsFraction);
    const float response = mode == 2 ? dynamic + ads * (t * t - dynamic) : t;
    const float scale = response / magnitude;
    pitch *= scale;
    yaw *= scale;
}

inline void RightStick(short x, short y, float inner, float outer, float& outX, float& outY)
{
    const float fx = static_cast<float>(x) / 32767.0f;
    const float fy = static_cast<float>(y) / 32767.0f;
    const float magnitude = std::sqrt(fx * fx + fy * fy);
    outX = outY = 0.0f;
    if (magnitude <= inner || magnitude == 0.0f) return;
    // An overlapping outer deadzone is saturated, never divided by zero.
    const float end = 1.0f - outer;
    const float response = magnitude >= end ? 1.0f : (magnitude - inner) / (end - inner);
    outX = fx * (response / magnitude);
    outY = fy * (response / magnitude);
}

inline float AdsScale(float fraction, float multiplier)
{
    return 1.0f + fraction * (multiplier - 1.0f);
}

} // namespace bo1::android::pad
