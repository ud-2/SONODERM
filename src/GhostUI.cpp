#include "GhostUI.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <cctype>

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

    style.WindowPadding     = ImVec2(18.0f, 20.0f);
    style.ItemSpacing       = ImVec2(10.0f, 8.0f);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg]             = ImVec4(0.03f, 0.04f, 0.07f, 0.88f);
    colors[ImGuiCol_ChildBg]              = ImVec4(0.00f, 0.00f, 0.00f, 0.20f);
    colors[ImGuiCol_PopupBg]              = ImVec4(0.04f, 0.05f, 0.08f, 0.92f);
    colors[ImGuiCol_Border]               = ImVec4(0.25f, 0.55f, 0.85f, 0.22f);

    colors[ImGuiCol_Text]                 = ImVec4(0.92f, 0.96f, 1.00f, 0.90f);
    colors[ImGuiCol_TextDisabled]         = ImVec4(0.40f, 0.46f, 0.58f, 0.65f);

    colors[ImGuiCol_Header]               = ImVec4(0.20f, 0.45f, 0.75f, 0.30f);
    colors[ImGuiCol_HeaderHovered]        = ImVec4(0.28f, 0.58f, 0.90f, 0.45f);
    colors[ImGuiCol_HeaderActive]         = ImVec4(0.35f, 0.70f, 1.00f, 0.60f);

    colors[ImGuiCol_Button]               = ImVec4(0.12f, 0.20f, 0.32f, 0.60f);
    colors[ImGuiCol_ButtonHovered]        = ImVec4(0.22f, 0.40f, 0.65f, 0.75f);
    colors[ImGuiCol_ButtonActive]         = ImVec4(0.30f, 0.55f, 0.85f, 0.90f);

    colors[ImGuiCol_SliderGrab]           = ImVec4(0.35f, 0.75f, 1.00f, 0.90f);
    colors[ImGuiCol_SliderGrabActive]     = ImVec4(0.55f, 0.90f, 1.00f, 1.00f);
    colors[ImGuiCol_FrameBg]              = ImVec4(0.08f, 0.12f, 0.18f, 0.65f);
    colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.12f, 0.18f, 0.28f, 0.80f);
    colors[ImGuiCol_FrameBgActive]        = ImVec4(0.18f, 0.26f, 0.40f, 0.95f);

    colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.00f, 0.00f, 0.00f, 0.0f);
    colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.30f, 0.45f, 0.65f, 0.25f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.60f, 0.85f, 0.45f);
    colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.50f, 0.75f, 1.00f, 0.70f);
}

UIActions GhostUI::render_interface(
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
) {
    UIActions actions;
    s_current_volume = current_volume;

    ImGuiIO& io = ImGui::GetIO();
    constexpr float VEIL_WIDTH = 360.0f;

    // 1. Proximity detection
    bool mouse_near_left_edge = (io.MousePos.x >= 0.0f && io.MousePos.x < 36.0f);
    bool mouse_inside_panel   = (io.MousePos.x >= 0.0f && io.MousePos.x < VEIL_WIDTH + 20.0f);
    bool target_open = s_veil_locked || mouse_near_left_edge || (s_veil_anim > 0.05f && mouse_inside_panel);

    // 2. Kinetic Spring Animation
    float speed = target_open ? 12.0f : 9.0f;
    float blend = 1.0f - std::exp(-speed * dt);
    s_veil_anim += ((target_open ? 1.0f : 0.0f) - s_veil_anim) * blend;

    // 3. Left Edge Pulsing Trigger
    if (s_veil_anim < 0.95f) {
        ImDrawList* bg_draw = ImGui::GetBackgroundDrawList();
        float pulse = 0.35f + 0.25f * std::sin(ImGui::GetTime() * 3.0f);
        bg_draw->AddRectFilled(
            ImVec2(0.0f, 40.0f),
            ImVec2(4.0f, io.DisplaySize.y - 40.0f),
            ImColor(0.35f, 0.75f, 1.0f, pulse)
        );
    }

    // 4. Render Obsidian Veil
    if (s_veil_anim > 0.01f) {
        float panel_x = (s_veil_anim - 1.0f) * VEIL_WIDTH;

        ImGui::SetNextWindowPos(ImVec2(panel_x, 0.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(VEIL_WIDTH, io.DisplaySize.y), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.90f * s_veil_anim);

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
            ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 0.85f), "NOW PLAYING [%zu/%zu]", track_idx + 1, playlist_items.size());
            ImGui::TextWrapped("%s", current_track_name.c_str());

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

            // Playback Buttons
            float button_width = 85.0f;
            float total_row_w = (button_width * 3.0f) + (ImGui::GetStyle().ItemSpacing.x * 2.0f);
            float offset_x = (VEIL_WIDTH - 36.0f - total_row_w) * 0.5f;
            if (offset_x > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset_x);

            if (ImGui::Button("|< Prev", ImVec2(button_width, 32.0f))) {
                actions.prev_track = true;
            }
            ImGui::SameLine();

            const char* play_pause_label = is_paused ? "Play >" : "Pause ||";
            if (ImGui::Button(play_pause_label, ImVec2(button_width, 32.0f))) {
                actions.toggle_play_pause = true;
            }
            ImGui::SameLine();

            if (ImGui::Button("Next >|", ImVec2(button_width, 32.0f))) {
                actions.next_track = true;
            }

            ImGui::Spacing();

            // Volume Slider
            ImGui::TextDisabled("VOLUME");
            ImGui::SameLine();
            ImGui::PushItemWidth(-1.0f);
            if (ImGui::SliderFloat("##VolSlider", &s_current_volume, 0.0f, 1.2f, "%.0f %%")) {
                actions.volume_change = s_current_volume;
            }
            ImGui::PopItemWidth();

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // -------------------------------------------------------------
            // Playlist Header & Instant Search
            // -------------------------------------------------------------
            ImGui::TextColored(ImVec4(0.35f, 0.75f, 1.0f, 0.90f), "PLAYLIST (%zu TRACKS)", playlist_items.size());

            ImGui::PushItemWidth(-1.0f);
            ImGui::InputTextWithHint("##TrackSearch", "Search tracks...", s_search_query, sizeof(s_search_query));
            ImGui::PopItemWidth();

            // Filter playlist items by query
            std::string query = s_search_query;
            std::transform(query.begin(), query.end(), query.begin(), [](unsigned char c){ return std::tolower(c); });

            std::vector<const PlaylistItem*> filtered;
            filtered.reserve(playlist_items.size());
            for (const auto& item : playlist_items) {
                if (query.empty()) {
                    filtered.push_back(&item);
                } else {
                    std::string lower_title = item.title;
                    std::transform(lower_title.begin(), lower_title.end(), lower_title.begin(), [](unsigned char c){ return std::tolower(c); });
                    if (lower_title.find(query) != std::string::npos) {
                        filtered.push_back(&item);
                    }
                }
            }

            ImGui::Spacing();

            // -------------------------------------------------------------
            // Virtualized Scrolling Playlist (ImGuiListClipper)
            // -------------------------------------------------------------
            ImGui::BeginChild("##PlaylistScrollArea", ImVec2(0.0f, 0.0f), true, ImGuiWindowFlags_None);

            ImGuiListClipper clipper;
            clipper.Begin(static_cast<int>(filtered.size()));

            while (clipper.Step()) {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
                    const PlaylistItem* item = filtered[i];
                    bool is_current = (item->index == track_idx);

                    ImGui::PushID(static_cast<int>(item->index));

                    // 1. Draw Song DNA Chromatic Badge
                    ImVec2 cursor = ImGui::GetCursorScreenPos();
                    float badge_radius = 4.5f;
                    ImVec2 badge_center(cursor.x + 8.0f, cursor.y + ImGui::GetTextLineHeight() * 0.5f);
                    ImDrawList* draw_list = ImGui::GetWindowDrawList();

                    draw_list->AddCircleFilled(
                        badge_center,
                        badge_radius,
                        ImColor(item->color_r, item->color_g, item->color_b, 0.95f)
                    );

                    if (is_current) {
                        draw_list->AddCircle(
                            badge_center,
                            badge_radius + 2.5f,
                            ImColor(0.40f, 0.85f, 1.00f, 0.85f),
                            0,
                            1.5f
                        );
                    }

                    // 2. Selectable Track Row
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 18.0f);

                    char item_label[256];
                    if (is_current) {
                        std::snprintf(item_label, sizeof(item_label), "> %zu. %s", item->index + 1, item->title.c_str());
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.40f, 0.85f, 1.00f, 1.0f));
                    } else {
                        std::snprintf(item_label, sizeof(item_label), "%zu. %s", item->index + 1, item->title.c_str());
                    }

                    if (ImGui::Selectable(item_label, is_current, ImGuiSelectableFlags_None, ImVec2(0, 0))) {
                        actions.select_track_index = static_cast<int>(item->index);
                    }

                    if (is_current) {
                        ImGui::PopStyleColor();
                    }

                    ImGui::PopID();
                }
            }

            ImGui::EndChild();
            ImGui::End();
        }
    }

    return actions;
}
