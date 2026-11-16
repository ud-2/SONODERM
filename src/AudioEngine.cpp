#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "AudioEngine.hpp"
#include <iostream>
#include <algorithm>

static void custom_device_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    (void)pInput;

    auto* engine = static_cast<AudioEngine*>(pDevice->pUserData);
    if (!engine || !pOutput) {
        return;
    }

    ma_engine_read_pcm_frames(engine->get_internal_engine(), pOutput, frameCount, nullptr);
    engine->push_samples(static_cast<const float*>(pOutput), frameCount, 2);
}

AudioEngine::AudioEngine() {
    m_engine = new ma_engine();
    m_sound  = new ma_sound();
    m_device = new ma_device();

    m_samples.resize(AUDIO_BUFFER_SIZE, 0.0f);
}

AudioEngine::~AudioEngine() {
    stop();

    if (m_device) {
        ma_device_uninit(m_device);
        delete m_device;
        m_device = nullptr;
    }

    if (m_is_initialized) {
        ma_engine_uninit(m_engine);
    }

    delete m_sound;
    delete m_engine;
}

bool AudioEngine::init() {
    ma_engine_config engineConfig = ma_engine_config_init();
    engineConfig.noDevice   = MA_TRUE;
    engineConfig.channels   = 2;
    engineConfig.sampleRate = 44100;

    ma_result result = ma_engine_init(&engineConfig, m_engine);
    if (result != MA_SUCCESS) {
        std::cerr << "Failed to initialize miniaudio engine. Code: " << result << "\n";
        return false;
    }

    ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format   = ma_format_f32;
    deviceConfig.playback.channels = 2;
    deviceConfig.sampleRate        = 44100;
    deviceConfig.dataCallback      = custom_device_data_callback;
    deviceConfig.pUserData         = this;

    result = ma_device_init(nullptr, &deviceConfig, m_device);
    if (result != MA_SUCCESS) {
        std::cerr << "Failed to initialize audio device. Code: " << result << "\n";
        return false;
    }

    result = ma_device_start(m_device);
    if (result != MA_SUCCESS) {
        std::cerr << "Failed to start audio device. Code: " << result << "\n";
        return false;
    }

    m_is_initialized = true;
    std::cout << "Audio device and capture pipeline initialized successfully.\n";
    return true;
}

bool AudioEngine::load_and_play(const std::string& filepath) {
    if (!m_is_initialized) {
        std::cerr << "Cannot play sound: AudioEngine is not initialized.\n";
        return false;
    }

    stop();

    ma_result result = ma_sound_init_from_file(
        m_engine,
        filepath.c_str(),
        MA_SOUND_FLAG_STREAM,
        nullptr,
        nullptr,
        m_sound
    );

    if (result != MA_SUCCESS) {
        std::cerr << "Failed to load audio track: " << filepath << " (Code: " << result << ")\n";
        return false;
    }

    m_is_sound_loaded = true;
    m_is_paused = false;

    ma_sound_set_looping(m_sound, MA_FALSE);

    result = ma_sound_start(m_sound);
    if (result != MA_SUCCESS) {
        std::cerr << "Failed to start playback.\n";
        return false;
    }

    std::cout << "Playing: " << filepath << "\n";
    return true;
}

void AudioEngine::stop() {
    if (m_is_sound_loaded) {
        ma_sound_stop(m_sound);
        ma_sound_uninit(m_sound);
        m_is_sound_loaded = false;
    }
}

bool AudioEngine::is_playing() const {
    if (!m_is_sound_loaded) return false;
    return ma_sound_is_playing(m_sound);
}

bool AudioEngine::has_ended() const {
    if (!m_is_sound_loaded || !m_sound || m_is_paused) return false;

    if (ma_sound_at_end(m_sound) == MA_TRUE) return true;

    float total = get_total_duration_seconds();
    if (total > 0.5f && get_current_time_seconds() >= total - 0.05f) {
        return true;
    }
    return false;
}

void AudioEngine::toggle_pause() {
    if (!m_is_sound_loaded) return;

    if (m_is_paused) {
        ma_sound_start(m_sound);
        m_is_paused = false;
        std::cout << "[Playback Resumed]\n";
    } else {
        ma_sound_stop(m_sound);
        m_is_paused = true;
        std::cout << "[Playback Paused]\n";
    }
}

void AudioEngine::seek_to_seconds(float seconds) {
    if (!m_is_sound_loaded || !m_sound) return;
    float total = get_total_duration_seconds();
    float clamped_seconds = std::clamp(seconds, 0.0f, total);
    ma_uint64 target_frame = static_cast<ma_uint64>(clamped_seconds * 44100.0f);
    ma_sound_seek_to_pcm_frame(m_sound, target_frame);
}

void AudioEngine::seek_to_progress(float progress_01) {
    float total = get_total_duration_seconds();
    seek_to_seconds(progress_01 * total);
}

void AudioEngine::set_volume(float volume) {
    if (m_sound) {
        ma_sound_set_volume(m_sound, std::clamp(volume, 0.0f, 1.5f));
    }
}

float AudioEngine::get_volume() const {
    if (!m_sound) return 1.0f;
    return ma_sound_get_volume(m_sound);
}

float AudioEngine::get_current_time_seconds() const {
    if (!m_is_sound_loaded) return 0.0f;
    float cursor_seconds = 0.0f;
    ma_sound_get_cursor_in_seconds(m_sound, &cursor_seconds);
    return cursor_seconds;
}

float AudioEngine::get_total_duration_seconds() const {
    if (!m_is_sound_loaded) return 0.0f;
    float length_seconds = 0.0f;
    ma_sound_get_length_in_seconds(m_sound, &length_seconds);
    return length_seconds;
}

float AudioEngine::get_progress_01() const {
    float total = get_total_duration_seconds();
    if (total <= 0.001f) return 0.0f;
    float progress = get_current_time_seconds() / total;
    return std::clamp(progress, 0.0f, 1.0f);
}

void AudioEngine::push_samples(const float* pFrames, size_t frameCount, size_t channels) {
    std::lock_guard<std::mutex> lock(m_buffer_mutex);

    if (frameCount >= AUDIO_BUFFER_SIZE) {
        size_t offset = frameCount - AUDIO_BUFFER_SIZE;
        for (size_t i = 0; i < AUDIO_BUFFER_SIZE; ++i) {
            size_t idx = (offset + i) * channels;
            m_samples[i] = (channels >= 2) ? (pFrames[idx] + pFrames[idx + 1]) * 0.5f : pFrames[idx];
        }
    } else {
        size_t shift = frameCount;
        size_t remaining = AUDIO_BUFFER_SIZE - shift;
        std::copy(m_samples.begin() + shift, m_samples.end(), m_samples.begin());

        for (size_t i = 0; i < frameCount; ++i) {
            size_t idx = i * channels;
            float mono = (channels >= 2) ? (pFrames[idx] + pFrames[idx + 1]) * 0.5f : pFrames[idx];
            m_samples[remaining + i] = mono;
        }
    }
}

std::vector<float> AudioEngine::get_waveform_samples() {
    std::lock_guard<std::mutex> lock(m_buffer_mutex);
    return m_samples;
}
