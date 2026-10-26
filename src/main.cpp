#include <iostream>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Shader.hpp"
#include "AudioEngine.hpp"
#include "AudioAnalyzer.hpp"
#include "MotionBrain.hpp"
#include "SongDNA.hpp"
#include "FeedbackBuffer.hpp"

// Dynamic window dimensions
static int g_window_width  = 1280;
static int g_window_height = 720;
static bool g_window_resized = false;

/// Global hook (audio) for GLFW event callbacks
static AudioEngine* g_audio_ptr = nullptr;

/// GLFW keyboard callback for Ghost UI controls
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)scancode; (void)mods; // ;)
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        } else if (key == GLFW_KEY_SPACE) {
            if (g_audio_ptr) {
                g_audio_ptr->toggle_pause();
            }
        }
    }
}

/// GLFW window resize callback: triggers whenever the user resizes or maximizes the window
static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    (void)window;
    if (width > 0 && height > 0) {
        g_window_width  = width;
        g_window_height = height;
        g_window_resized = true;
    }
}

int main(int argc, char* argv[]) {
    // Default path I set for test
    std::string audio_path = "assets/audio/shinning_star.mp3";
    if (argc > 1) {
        audio_path = argv[1];
    }

    SongUniverse universe = SongDNABuilder::extract_dna(audio_path);

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    // Using OpenGL 3.3 Core
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    std::string window_title = "Sonoderm - " + universe.track_name;
    GLFWwindow* window = glfwCreateWindow(
        g_window_width, g_window_height, window_title.c_str(), nullptr, nullptr
    );

    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Register event listeners
    glfwSetKeyCallback(window, key_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    // Query actual initial pixel dimensions from the window
    // (handles HiDPI scaling since I didn't use glfwGetWindowSize function)
    glfwGetFramebufferSize(window, &g_window_width, &g_window_height);

    AudioEngine audio;

    if (!audio.init()) {
        std::cerr << "Audio initialization failed, running in silent mode.\n";
    } else {
        audio.load_and_play(audio_path);
    }

    g_audio_ptr = &audio; // The previous audio hook defined at the top of the file.

    AudioAnalyzer analyzer;
    MotionBrain brain;

    // Defines a full quad
    const float quad_vertices[] = {
        // lower-left triangle
        -1.0f,  1.0f,
        -1.0f, -1.0f,
         1.0f, -1.0f,

        // upper-right triangle
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

    std::cout << "\n= = = = =\n";
    std::cout << "SONODERM ENGINE RUNNING\n";
    std::cout << "[SPACE] : Play / Pause\n";
    std::cout << "[ESC/Q] : Quit\n";
    std::cout << "[Resize Window / Fullscreen supported]\n";
    std::cout << "= = = = =\n\n";

    {
        // Wrap shader's def to avoid calling glfwDestroyWindow below on a non-existing window because of ~Shader's def
        // Maybe, sound.frag is all the work
        Shader raymarch_shader("assets/shaders/quad.vert", "assets/shaders/sound.frag");
        Shader screen_shader("assets/shaders/quad.vert", "assets/shaders/screen.frag");

        FeedbackBuffer feedback(g_window_width, g_window_height);

        double last_frame_time = glfwGetTime();

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            // Handle dynamic window resizing
            if (g_window_resized) {
                feedback.resize(g_window_width, g_window_height);
                g_window_resized = false;
            }

            double current_frame_time = glfwGetTime();
            float dt = static_cast<float>(current_frame_time - last_frame_time);
            last_frame_time = current_frame_time;
            if (dt > 0.1f) dt = 0.1f;

            // Audio & Brain processing
            std::vector<float> waveform = audio.get_waveform_samples();
            FrequencyBands bands = analyzer.analyze(waveform);

            if (audio.is_paused()) {
                bands = FrequencyBands{};
            }

            brain.update(bands, dt);
            const VisualParameters& params = brain.get_params();

            float progress = audio.get_progress_01();
            float is_paused_f = audio.is_paused() ? 1.0f : 0.0f;

            // PASS 1: Render 3D Scene to offscreen framebuffer
            feedback.bind_write();

            raymarch_shader.use();
            feedback.bind_read_texture(0);
            raymarch_shader.set_int("u_prev_frame", 0);

            // Pass dynamic window dimensions
            raymarch_shader.set_vec2("u_resolution", static_cast<float>(g_window_width), static_cast<float>(g_window_height));
            raymarch_shader.set_float("u_audio_time", params.audio_time);
            raymarch_shader.set_float("u_bass_impact", params.bass_impact);
            raymarch_shader.set_float("u_mid_swell", params.mid_swell);
            raymarch_shader.set_float("u_treble_spark", params.treble_spark);
            raymarch_shader.set_float("u_energy", params.energy);

            raymarch_shader.set_float("u_progress", progress);
            raymarch_shader.set_float("u_is_paused", is_paused_f);

            raymarch_shader.set_vec3("u_pal_a", universe.palette.a[0], universe.palette.a[1], universe.palette.a[2]);
            raymarch_shader.set_vec3("u_pal_b", universe.palette.b[0], universe.palette.b[1], universe.palette.b[2]);
            raymarch_shader.set_vec3("u_pal_c", universe.palette.c[0], universe.palette.c[1], universe.palette.c[2]);
            raymarch_shader.set_vec3("u_pal_d", universe.palette.d[0], universe.palette.d[1], universe.palette.d[2]);

            raymarch_shader.set_float("u_shape_type", universe.geometry.core_shape_type);
            raymarch_shader.set_float("u_satellites", universe.geometry.satellite_count);
            raymarch_shader.set_float("u_twist", universe.geometry.twist_strength);
            raymarch_shader.set_float("u_roughness", universe.geometry.roughness_factor);

            // Print full quad
            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            // PASS 2: Blit directly to screen at current dynamic resolution
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, g_window_width, g_window_height);

            screen_shader.use();

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, feedback.get_current_texture());

            screen_shader.set_int("u_screen_texture", 0);
            screen_shader.set_vec2("u_resolution", static_cast<float>(g_window_width), static_cast<float>(g_window_height));

            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);

            // Every turn, for smooth effect
            feedback.swap();
            glfwSwapBuffers(window);
        }

        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
    }

    g_audio_ptr = nullptr;
    audio.stop();
    glfwDestroyWindow(window);
    glfwTerminate();

    std::cout << "Sonoderm closed cleanly.\n";
    return 0;
}