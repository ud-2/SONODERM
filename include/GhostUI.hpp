#pragma once

#include <string>

struct GLFWwindow;

// Dispatched actions from UI to audio and playback system
struct UIActions {
    bool toggle_play_pause = false;
    bool next_track        = false;
    bool prev_track        = false;
    float seek_target      = -1.0f; // Range [0.0, 1.0], or < 0 if no seek request
    float volume_change    = -1.0f; // Range [0.0, 1.0], or < 0 if no volume request
};

class GhostUI {
public:
    static bool init(GLFWwindow* window);
    static void shutdown();
    static void begin_frame();
    static void end_frame();

    // Toggle Veil panel visibility lock (via TAB key)
    static void toggle_veil();

    // Render the Obsidian Veil and interactive controls
    static UIActions render_interface(
        size_t track_idx,
        size_t total_tracks,
        const std::string& track_name,
        const std::string& folder_name,
        float progress,
        float current_time_sec,
        float total_time_sec,
        float current_volume,
        bool is_paused,
        float dt,
        float fps
    );

private:
    static void apply_ghost_theme();
    static std::string format_time(float seconds);

    // Veil state & kinetic animation interpolation
    static inline bool  s_veil_locked   = false;
    static inline float s_veil_anim     = 0.0f; // Range [0.0 (hidden), 1.0 (opened)]
    static inline float s_current_volume = 1.0f;
};
