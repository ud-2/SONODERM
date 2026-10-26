#pragma once

#include "AudioAnalyzer.hpp"

#include <cmath>
#include <algorithm>

// Smooth, physically grounded parameters ready to feed GPU uniforms
struct VisualParameters {
    float bass_impact = 0.0f;
    float mid_swell = 0.0f;
    float treble_spark = 0.0f;

    float audio_time = 0.0f; // Dynamic time accumulator driven by musical energy
    float energy = 0.0f; // Overall music energy level [0.0, 1.0]
};

class MotionBrain {
public:
    MotionBrain() = default;

    // Call once per frame with the raw frequency bands and frame delta time (dt)
    void update(const FrequencyBands& bands, float dt);
    const VisualParameters& get_params() const { return m_params; }

private:
    VisualParameters m_params;

    // Smooth exponential interpolation with distinct attack and decay rates
    float smooth_spring(float current, float target, float attack_speed, float decay_speed, float dt);
};