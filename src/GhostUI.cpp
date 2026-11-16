#include "GhostUI.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

bool GhostUI::init(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // Setup base IO flags
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Apply our bespoke non-intrusive transparent theme
    apply_ghost_theme();

    // Initialize backend bindings
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

void GhostUI::apply_ghost_theme() {
    ImGuiStyle& style = ImGui::GetStyle();

    // Rounding & Spacing
    style.WindowRounding    = 8.0f;
    style.FrameRounding     = 6.0f;
    style.PopupRounding     = 8.0f;
    style.ScrollbarRounding = 12.0f;
    style.GrabRounding      = 6.0f;

    style.WindowBorderSize  = 0.0f;
    style.FrameBorderSize   = 0.0f;
    style.PopupBorderSize   = 0.0f;

    style.WindowPadding     = ImVec2(16.0f, 16.0f);
    style.ItemSpacing       = ImVec2(10.0f, 8.0f);

    // Pure Ghost Palette (Transparent obsidian, zero solid gray)
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg]             = ImVec4(0.04f, 0.05f, 0.08f, 0.0f); // 100% transparent default
    colors[ImGuiCol_ChildBg]              = ImVec4(0.00f, 0.00f, 0.00f, 0.0f);
    colors[ImGuiCol_PopupBg]              = ImVec4(0.05f, 0.06f, 0.10f, 0.85f);
    colors[ImGuiCol_Border]               = ImVec4(1.00f, 1.00f, 1.00f, 0.08f);

    // Text & Accents
    colors[ImGuiCol_Text]                 = ImVec4(0.92f, 0.95f, 1.00f, 0.85f);
    colors[ImGuiCol_TextDisabled]         = ImVec4(0.40f, 0.45f, 0.55f, 0.60f);

    // Headers & Buttons
    colors[ImGuiCol_Header]               = ImVec4(0.20f, 0.35f, 0.55f, 0.25f);
    colors[ImGuiCol_HeaderHovered]        = ImVec4(0.30f, 0.50f, 0.80f, 0.40f);
    colors[ImGuiCol_HeaderActive]         = ImVec4(0.40f, 0.65f, 0.95f, 0.50f);

    colors[ImGuiCol_Button]               = ImVec4(0.15f, 0.25f, 0.40f, 0.30f);
    colors[ImGuiCol_ButtonHovered]        = ImVec4(0.25f, 0.45f, 0.70f, 0.50f);
    colors[ImGuiCol_ButtonActive]         = ImVec4(0.35f, 0.60f, 0.90f, 0.65f);

    colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.40f, 0.50f, 0.70f, 0.20f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.50f, 0.65f, 0.85f, 0.40f);
    colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.60f, 0.80f, 1.00f, 0.60f);
}

void GhostUI::render_test_overlay(size_t track_idx, size_t total_tracks, const std::string& track_name, float fps) {
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration 
                           | ImGuiWindowFlags_AlwaysAutoResize 
                           | ImGuiWindowFlags_NoSavedSettings 
                           | ImGuiWindowFlags_NoFocusOnAppearing 
                           | ImGuiWindowFlags_NoNav 
                           | ImGuiWindowFlags_NoMove;

    // Pin to top-left margin
    ImGui::SetNextWindowPos(ImVec2(24.0f, 24.0f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.25f); // Soft glass backdrop

    if (ImGui::Begin("##GhostOverlayTest", nullptr, flags)) {
        ImGui::TextColored(ImVec4(0.35f, 0.75f, 1.0f, 0.9f), "SONODERM GHOST UI");
        ImGui::Text("Track [%zu/%zu] : %s", track_idx + 1, total_tracks, track_name.c_str());
        ImGui::TextColored(ImVec4(0.5f, 0.9f, 0.5f, 0.8f), "Engine Performance: %.1f FPS", fps);
        ImGui::Separator();
        ImGui::TextDisabled("[TAB] : Obsidian Veil (Action A2)");
        ImGui::End();
    }
}
