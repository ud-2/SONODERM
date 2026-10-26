#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <iostream>
#include <algorithm>

// I may change this later. But for now, let's keep it simple for a naive waveform visualization
constexpr size_t AUDIO_BUFFER_SIZE = 512;

// Forward declarations since I use MA pointers
struct ma_engine;
struct ma_sound;
struct ma_device;

class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

    bool init();
    bool load_and_play(const std::string& filepath);
    void stop();
    bool is_playing() const;

    // Playback control
    void toggle_pause();
    bool is_paused() const { return m_is_paused; }

    // Track timeline progress in seconds
    float get_current_time_seconds() const;
    float get_total_duration_seconds() const;
    float get_progress_01() const; // Normalized progress towards [0.0, 1.0]

    std::vector<float> get_waveform_samples();
    void push_samples(const float* pFrames, size_t frameCount, size_t channels);

    ma_engine* get_internal_engine() const { return m_engine; }

private:
    ma_engine* m_engine = nullptr;
    ma_sound*  m_sound  = nullptr;
    ma_device* m_device = nullptr;

    bool m_is_initialized  = false;
    bool m_is_sound_loaded = false;
    bool m_is_paused       = false;

    mutable std::mutex m_buffer_mutex; // Just to read m_samples safely between threads
    std::vector<float> m_samples;
};