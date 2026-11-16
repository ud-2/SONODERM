#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <filesystem>
#include <algorithm>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Shader.hpp"
#include "AudioEngine.hpp"
#include "AudioAnalyzer.hpp"
#include "MotionBrain.hpp"
#include "SongDNA.hpp"
#include "FeedbackBuffer.hpp"
#include "GhostUI.hpp"

namespace fs = std::filesystem;

static int g_window_width  = 1280;
static int g_window_height = 720;
static bool g_window_resized = false;

static AudioEngine* g_audio_ptr = nullptr;
static int g_track_skip_request = 0;

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)scancode; (void)mods;
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        } else if (key == GLFW_KEY_SPACE) {
            if (g_audio_ptr) {
                g_audio_ptr->toggle_pause();
            }
        } else if (key == GLFW_KEY_RIGHT || key == GLFW_KEY_N) {
            g_track_skip_request = 1;
        } else if (key == GLFW_KEY_LEFT || key == GLFW_KEY_P) {
            g_track_skip_request = -1;
        }
    }
}

static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    (void)window;
    if (width > 0 && height > 0) {
        g_window_width  = width;
        g_window_height = height;
        g_window_resized = true;
    }
}

int main(int argc, char* argv[]) {
    std::string input_path = "assets/audio";
    if (argc > 1) {
        input_path = argv[1];
    }

    std::vector<std::string> playlist;
    std::error_code ec;

    if (fs::is_directory(input_path, ec)) {
        for (const auto& entry : fs::directory_iterator(input_path, ec)) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext == ".mp3" || ext == ".wav" || ext == ".flac" || ext == ".ogg") {
                    playlist.push_back(entry.path().string());
                }
            }
        }
        std::sort(playlist.begin(), playlist.end());
    } else if (fs::is_regular_file(input_path, ec)) {
        playlist.push_back(input_path);
    }

    if (playlist.empty()) {
        std::cerr << "Error: No playable audio files found in: " << input_path << "\n";
        return -1;
    }

    size_t current_track_idx = 0;
    SongUniverse universe = SongDNABuilder::extract_dna(playlist[current_track_idx]);

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    auto format_window_title = [&](size_t idx, const SongUniverse& u) {
        return "Sonoderm - [" + std::to_string(idx + 1) + "/" +
               std::to_string(playlist.size()) + "] " + u.track_name;
    };

    std::string initial_title = format_window_title(current_track_idx, universe);
    GLFWwindow* window = glfwCreateWindow(
        g_window_width, g_window_height, initial_title.c_str(), nullptr, nullptr
    );

    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, key_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    glfwGetFramebufferSize(window, &g_window_width, &g_window_height);

    // Initialize Ghost UI Engine
    if (!GhostUI::init(window)) {
        std::cerr << "Failed to initialize Ghost UI\n";
    }

    AudioEngine   audio;
    g_audio_ptr = &audio;

    AudioAnalyzer analyzer;
    MotionBrain   brain;

    if (!audio.init()) {
        std::cerr << "Audio initialization failed, running in silent mode.\n";
    } else {
        audio.load_and_play(playlist[current_track_idx]);
    }

    const float quad_vertices[] = {
        -1.0f,  1.0f,
        -1.0f, -1.0f,
         1.0f, -1.0f,

        -1.0f,  1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f
    };

    unsigned int vao = 0;
    unsigned int vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), static_cast<void*>(0));
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    {
        // Notice sound.frag is used here!
        Shader raymarch_shader("assets/shaders/quad.vert", "assets/shaders/sound.frag");
        Shader screen_shader("assets/shaders/quad.vert", "assets/shaders/screen.frag");

        FeedbackBuffer feedback(g_window_width, g_window_height);

        double last_frame_time = glfwGetTime();

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            bool change_track = false;
            if (g_track_skip_request != 0) {
                if (g_track_skip_request > 0) {
                    current_track_idx = (current_track_idx + 1) % playlist.size();
                } else {
                    current_track_idx = (current_track_idx + playlist.size() - 1) % playlist.size();
                }
                g_track_skip_request = 0;
                change_track = true;
            } else if (audio.has_ended()) {
                current_track_idx = (current_track_idx + 1) % playlist.size();
                change_track = true;
            }

            if (change_track) {
                const std::string& next_track = playlist[current_track_idx];
                universe = SongDNABuilder::extract_dna(next_track);
                audio.load_and_play(next_track);
                std::string new_title = format_window_title(current_track_idx, universe);
                glfwSetWindowTitle(window, new_title.c_str());
            }

            if (g_window_resized) {
                feedback.resize(g_window_width, g_window_height);
                g_window_resized = false;
            }

            double current_frame_time = glfwGetTime();
            float dt = static_cast<float>(current_frame_time - last_frame_time);
            last_frame_time = current_frame_time;
            if (dt > 0.1f) dt = 0.1f;

            std::vector<float> waveform = audio.get_waveform_samples();
            FrequencyBands bands = analyzer.analyze(waveform);

            if (audio.is_paused()) {
                bands = FrequencyBands{};
            }

            brain.update(bands, dt);
            const VisualParameters& params = brain.get_params();

            float progress = audio.get_progress_01();
            float is_paused_f = audio.is_paused() ? 1.0f : 0.0f;

            // PASS 1: Raymarch 3D Scene + Accumulate into Offscreen FBO
            feedback.bind_write();

            raymarch_shader.use();
            feedback.bind_read_texture(0);
            raymarch_shader.set_int("u_prev_frame", 0);

            raymarch_shader.set_vec2("u_resolution", static_cast<float>(g_window_width), static_cast<float>(g_window_height));
            raymarch_shader.set_float("u_audio_time",   params.audio_time);
            raymarch_shader.set_float("u_bass_impact",  params.bass_impact);
            raymarch_shader.set_float("u_mid_swell",    params.mid_swell);
            raymarch_shader.set_float("u_treble_spark", params.treble_spark);
            raymarch_shader.set_float("u_energy",       params.energy);

            raymarch_shader.set_float("u_progress",  progress);
            raymarch_shader.set_float("u_is_paused", is_paused_f);

            raymarch_shader.set_vec3("u_pal_a", universe.palette.a[0], universe.palette.a[1], universe.palette.a[2]);
            raymarch_shader.set_vec3("u_pal_b", universe.palette.b[0], universe.palette.b[1], universe.palette.b[2]);
            raymarch_shader.set_vec3("u_pal_c", universe.palette.c[0], universe.palette.c[1], universe.palette.c[2]);
            raymarch_shader.set_vec3("u_pal_d", universe.palette.d[0], universe.palette.d[1], universe.palette.d[2]);

            raymarch_shader.set_float("u_shape_type",  universe.geometry.core_shape_type);
            raymarch_shader.set_float("u_satellites",  universe.geometry.satellite_count);
            raymarch_shader.set_float("u_twist",       universe.geometry.twist_strength);
            raymarch_shader.set_float("u_roughness",   universe.geometry.roughness_factor);

            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            // PASS 2: Blit directly to Screen
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, g_window_width, g_window_height);

            screen_shader.use();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, feedback.get_current_texture());
            screen_shader.set_int("u_screen_texture", 0);
            screen_shader.set_vec2("u_resolution", static_cast<float>(g_window_width), static_cast<float>(g_window_height));

            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);

            // PASS 3: Ghost UI Render on Top (Does not contaminate temporal feedback buffer!)
            float current_fps = 1.0f / (dt > 0.0001f ? dt : 0.016f);
            GhostUI::begin_frame();
            GhostUI::render_test_overlay(current_track_idx, playlist.size(), universe.track_name, current_fps);
            GhostUI::end_frame();

            feedback.swap();
            glfwSwapBuffers(window);
        }

        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
    }

    GhostUI::shutdown();
    g_audio_ptr = nullptr;
    audio.stop();
    glfwDestroyWindow(window);
    glfwTerminate();

    std::cout << "Sonoderm closed cleanly.\n";
    return 0;
}
