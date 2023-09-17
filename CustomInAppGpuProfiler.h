#pragma once

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <cstdio>

#define GPU_CONTEXT GLFWwindow*

static GPU_CONTEXT GetCurrentContext() {
    return glfwGetCurrentContext();
}
#define GET_CURRENT_CONTEXT GetCurrentContext

static void SetCurrentContext(GPU_CONTEXT vContext) {
    glfwMakeContextCurrent(vContext);
}
#define SET_CURRENT_CONTEXT SetCurrentContext

static bool PlayPauseButton(bool& vIsPaused) {
    bool res = false;
    const char* play_pause_label = "Pause";
    if (vIsPaused) {
        play_pause_label = "Play";
    }
    if (ImGui::Button(play_pause_label)) {
        vIsPaused = !vIsPaused;
        res = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Play/Pause Profiling");
    }
    return res;
}
#define IMGUI_PLAY_PAUSE_BUTTON PlayPauseButton

#define LOG_ERROR_MESSAGE printf

#define LOG_DEBUG_ERROR_MESSAGE printf
