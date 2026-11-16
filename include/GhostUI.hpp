#pragma once

#include <string>
#include <vector>

struct GLFWwindow;

// Lightweight representation of a playlist item with its DNA preview color
struct PlaylistItem {
    size_t index = 0;
    std::string title;
    float color_r = 0.5f;
    float color_g = 0.5f;
    float color_b = 0.5f;
};

// Dispatched actions from UI to audio and playback system
struct UIActions {
    bool toggle_play_pause  = false;
    bool next_track         = false;
    bool prev_track         = false;
    int select_track_index  = -1;    // >= 0 if user clicked a specific track in the list
    float seek_target       = -1.0f; // Range [0.0, 1.0], or < 0 if no seek request
    float volume_change     = -1.0f; // Range [0.0, 1.0], or < 0 if no volume request
};

class GhostUI {
public:
    static bool init(GLFWwindow* window);
    static void shutdown();
    static void begin_frame();
    static void end_frame();

    static void toggle_veil();

    static UIActions render_interface(
        size_t track_idx,
        const std::vector<PlaylistItem>& playlist_items,
        const std::string& current_track_name,
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

    static inline bool  s_veil_locked       = false;
    static inline float s_veil_anim         = 0.0f;
    static inline float s_current_volume    = 1.0f;
    static inline char  s_search_query[128] = "";
};
