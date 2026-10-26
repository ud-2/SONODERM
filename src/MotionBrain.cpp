#include "MotionBrain.hpp"

/**
 * @brief Smoothly interpolates a value toward a target using exponential smoothing.
 *
 * @param current      The current value.
 * @param target       The target value to approach.
 * @param attack_speed The convergence rate when the target exceeds the current
 *                     value, expressed in inverse seconds (s^-1).
 * @param decay_speed  The convergence rate when the target is less than or
 *                     equal to the current value, expressed in inverse
 *                     seconds (s^-1).
 * @param dt           The elapsed time since the previous update, in seconds.
 *
 * @return The updated value, smoothly interpolated toward the target.
 *
 * @see https://en.wikipedia.org/wiki/Exponential_smoothing
 */
float MotionBrain::smooth_spring(float current, float target, float attack_speed, float decay_speed, float dt) {
    float speed = (target > current) ? attack_speed : decay_speed;
    float blend = 1.0f - std::exp(-speed * dt);

    return current + (target - current) * blend;
}

void MotionBrain::update(const FrequencyBands& bands, float dt) {
    // Compute composite instantaneous energy
    // Align on 50%, 35%, 15% contributions from bass, mid, and treble respectively
    float instant_energy = bands.bass * 0.5f + bands.mid * 0.35f + bands.treble * 0.15f;
    m_params.energy = smooth_spring(m_params.energy, instant_energy, 12.0f, 3.0f, dt);

    // Accumlate audio time based on the energy.
    // - Base speed = 0.25x
    // - High speed = 2.45x
    float time_rate = 0.25f + (m_params.energy * 2.2f);
    m_params.audio_time += dt * time_rate;

    m_params.bass_impact = smooth_spring(m_params.bass_impact, bands.bass, 35.0f, 6.0f, dt);
    m_params.mid_swell = smooth_spring(m_params.mid_swell, bands.mid, 10.0f, 4.0f, dt);
    m_params.treble_spark = smooth_spring(m_params.treble_spark, bands.treble, 40.0f, 15.0f, dt);
}