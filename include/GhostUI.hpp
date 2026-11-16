#pragma once

#include <string>

struct GLFWwindow;

class GhostUI {
public:
    static bool init(GLFWwindow* window);
    static void shutdown();
    static void begin_frame();
    static void end_frame();

    // Action A1 Test Overlay: displays floating transparent diagnostic info
    static void render_test_overlay(size_t track_idx, size_t total_tracks, const std::string& track_name, float fps);

private:
    static void apply_ghost_theme();
};
