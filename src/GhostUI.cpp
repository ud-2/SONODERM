#include "GhostUI.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

bool GhostUI::init(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    apply_ghost_theme();

    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
        return false;
    }
    if (!ImGui_ImplOpenGL3_Init("#version 330 core")) {
        return false;
    }

    return true;
}

void GhostUI::shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void GhostUI::begin_frame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void GhostUI::end_frame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void GhostUI::toggle_veil() {
    s_veil_locked = !s_veil_locked;
}

std::string GhostUI::format_time(float seconds) {
    int total_s = static_cast<int>(seconds);
    if (total_s < 0) total_s = 0;
    int mins = total_s / 60;
    int secs = total_s % 60;
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%02d:%02d", mins, secs);
    return std::string(buf);
}

void GhostUI::apply_ghost_theme() {
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding    = 10.0f;
    style.FrameRounding     = 6.0f;
    style.PopupRounding     = 8.0f;
    style.ScrollbarRounding = 12.0f;
    style.GrabRounding      = 6.0f;

    style.WindowBorderSize  = 1.0f;
    style.FrameBorderSize   = 0.0f;

    style.WindowPadding     = ImVec2(20.0f, 24.0f);
    style.ItemSpacing       = ImVec2(12.0f, 10.0f);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg]             = ImVec4(0.03f, 0.04f, 0.07f, 0.85f); // Obsidian translucent
    colors[ImGuiCol_ChildBg]              = ImVec4(0.00f, 0.00f, 0.00f, 0.0f);
    colors[ImGuiCol_PopupBg]              = ImVec4(0.04f, 0.05f, 0.08f, 0.92f);
    colors[ImGuiCol_Border]               = ImVec4(0.25f, 0.55f, 0.85f, 0.20f); // Sleek cyan border

    colors[ImGuiCol_Text]                 = ImVec4(0.92f, 0.96f, 1.00f, 0.90f);
    colors[ImGuiCol_TextDisabled]         = ImVec4(0.40f, 0.46f, 0.58f, 0.65f);

    // Cyan glowing interactive widgets
    colors[ImGuiCol_Header]               = ImVec4(0.20f, 0.45f, 0.75f, 0.30f);
    colors[ImGuiCol_HeaderHovered]        = ImVec4(0.30f, 0.60f, 0.95f, 0.45f);
    colors[ImGuiCol_HeaderActive]         = ImVec4(0.40f, 0.75f, 1.00f, 0.60f);

    colors[ImGuiCol_Button]               = ImVec4(0.12f, 0.20f, 0.32f, 0.60f);
    colors[ImGuiCol_ButtonHovered]        = ImVec4(0.22f, 0.40f, 0.65f, 0.75f);
    colors[ImGuiCol_ButtonActive]         = ImVec4(0.30f, 0.55f, 0.85f, 0.90f);

    colors[ImGuiCol_SliderGrab]           = ImVec4(0.35f, 0.75f, 1.00f, 0.90f);
    colors[ImGuiCol_SliderGrabActive]     = ImVec4(0.55f, 0.90f, 1.00f, 1.00f);
    colors[ImGuiCol_FrameBg]              = ImVec4(0.08f, 0.12f, 0.18f, 0.60f);
    colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.12f, 0.18f, 0.28f, 0.75f);
    colors[ImGuiCol_FrameBgActive]        = ImVec4(0.18f, 0.26f, 0.40f, 0.90f);

    colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.00f, 0.00f, 0.00f, 0.0f);
    colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.30f, 0.45f, 0.65f, 0.25f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.60f, 0.85f, 0.45f);
    colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.50f, 0.75f, 1.00f, 0.70f);
}

UIActions GhostUI::render_interface(
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
) {
    UIActions actions;
    s_current_volume = current_volume;

    ImGuiIO& io = ImGui::GetIO();
    constexpr float VEIL_WIDTH = 340.0f;

    // 1. Proximity detection: mouse near left edge triggers opening
    bool mouse_near_left_edge = (io.MousePos.x >= 0.0f && io.MousePos.x < 36.0f);
    bool mouse_inside_panel   = (io.MousePos.x >= 0.0f && io.MousePos.x < VEIL_WIDTH + 20.0f);

    bool target_open = s_veil_locked || mouse_near_left_edge || (s_veil_anim > 0.05f && mouse_inside_panel);

    // 2. Kinetic Spring Animation (Independent of framerate)
    float speed = target_open ? 12.0f : 9.0f;
    float blend = 1.0f - std::exp(-speed * dt);
    s_veil_anim += ((target_open ? 1.0f : 0.0f) - s_veil_anim) * blend;

    // 3. Draw Edge Trigger Line if Veil is collapsed
    if (s_veil_anim < 0.95f) {
        ImDrawList* bg_draw = ImGui::GetBackgroundDrawList();
        float pulse = 0.35f + 0.25f * std::sin(ImGui::GetTime() * 3.0f);
        bg_draw->AddRectFilled(
            ImVec2(0.0f, 40.0f),
            ImVec2(4.0f, io.DisplaySize.y - 40.0f),
            ImColor(0.35f, 0.75f, 1.0f, pulse)
        );
    }

    // 4. Render Obsidian Veil if partially or fully visible
    if (s_veil_anim > 0.01f) {
        float panel_x = (s_veil_anim - 1.0f) * VEIL_WIDTH;

        ImGui::SetNextWindowPos(ImVec2(panel_x, 0.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(VEIL_WIDTH, io.DisplaySize.y), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.88f * s_veil_anim);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar 
                               | ImGuiWindowFlags_NoResize 
                               | ImGuiWindowFlags_NoMove 
                               | ImGuiWindowFlags_NoCollapse;

        if (ImGui::Begin("##ObsidianVeil", nullptr, flags)) {
            // Header
            ImGui::TextColored(ImVec4(0.35f, 0.75f, 1.0f, 1.0f), "SONODERM JUKEBOX");
            ImGui::SameLine();
            ImGui::TextDisabled("[TAB]");

            ImGui::TextDisabled("SOURCE : %s", folder_name.c_str());
            ImGui::TextColored(ImVec4(0.5f, 0.9f, 0.5f, 0.75f), "%.1f FPS", fps);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Current Track Card
            ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 0.85f), "NOW PLAYING [%zu/%zu]", track_idx + 1, total_tracks);
            ImGui::TextWrapped("%s", track_name.c_str());

            ImGui::Spacing();

            // Timeline Scrubbing Bar
            std::string time_str = format_time(current_time_sec) + " / " + format_time(total_time_sec);
            ImGui::TextDisabled("%s", time_str.c_str());

            float scrub_progress = progress;
            ImGui::PushItemWidth(-1.0f);
            if (ImGui::SliderFloat("##SeekSlider", &scrub_progress, 0.0f, 1.0f, "")) {
                actions.seek_target = scrub_progress;
            }
            ImGui::PopItemWidth();

            ImGui::Spacing();

            // Visual Playback Control Buttons (Centered row)
            float button_width = 80.0f;
            float total_row_w = (button_width * 3.0f) + (ImGui::GetStyle().ItemSpacing.x * 2.0f);
            float offset_x = (VEIL_WIDTH - 40.0f - total_row_w) * 0.5f;
            if (offset_x > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset_x);

            if (ImGui::Button("|< Prev", ImVec2(button_width, 34.0f))) {
                actions.prev_track = true;
            }
            ImGui::SameLine();

            const char* play_pause_label = is_paused ? "Play >" : "Pause ||";
            if (ImGui::Button(play_pause_label, ImVec2(button_width, 34.0f))) {
                actions.toggle_play_pause = true;
            }
            ImGui::SameLine();

            if (ImGui::Button("Next >|", ImVec2(button_width, 34.0f))) {
                actions.next_track = true;
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Volume Slider
            ImGui::TextDisabled("VOLUME");
            ImGui::PushItemWidth(-1.0f);
            if (ImGui::SliderFloat("##VolSlider", &s_current_volume, 0.0f, 1.2f, "%.0f %%")) {
                actions.volume_change = s_current_volume;
            }
            ImGui::PopItemWidth();

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Veil lock status hint
            if (s_veil_locked) {
                ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 0.8f), "* Panel Pinned Open");
            } else {
                ImGui::TextDisabled("* Auto-collapse enabled");
            }
            ImGui::TextDisabled("Press TAB to pin / unpin");

            ImGui::End();
        }
    }

    return actions;
}
