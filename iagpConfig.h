#pragma once

////////////////////////////////////////////////////////////////
// YOU NEED TO DEFINE THESES VARS FOR USING IAGP ///////////////
// you can check the demo app for an example use ///////////////
// https://github.com/aiekick/InAppGpuProfiler/tree/DemoApp ////
////////////////////////////////////////////////////////////////

// you need also to put your opengl laoder here
// without that you will have many linker error
// ex #include <glald/glad.h>

// you need also to put your imgui header path too
// without that you will have many linker error
// ex #include <glad/glad.h>

// you need also to put your context dependencies here if requried 
// by your get/set context fucntions
// without that you will have many linker error
// #define IAGP_GPU_CONTEXT void*
// #define IAGP_GET_CURRENT_CONTEXT
// #define IAGP_SET_CURRENT_CONTEXT SetCurrentContext

////////////////////////////////////////////////////////////////
// OPTIONNAL ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

// the title of the profiler detail imgui windows
//#define IAGP_DETAILS_TITLE "Profiler Details"

// the max level of recursion for profiler queries
//#define IAGP_RECURSIVE_LEVELS_COUNT 20U

// the mean average level 
// all the values will be smoothed on 60 frames (1s of 60fps diosplay)
//#define IAGP_MEAN_AVERAGE_LEVELS_COUNT 60U

//the minimal size of imgui sub window, when you openif by click right on a progiler bar
//#define IAGP_SUB_WINDOW_MIN_SIZE ImVec2(300, 100)

// the imgui button to use in IAGP
//#define IAGP_IMGUI_BUTTON ImGui::Button

// the Imgui Play/Pause button to use
//#define IAGP_IMGUI_PLAY_PAUSE_BUTTON
//#define IAGP_IMGUI_PLAY_LABEL "Play"
//#define IAGP_IMGUI_PAUSE_LABEL "Pause"
//#define IAGP_IMGUI_PLAY_PAUSE_HELP "Play/Pause Profiling"

// define your fucntion for log error message of IAGP
//#define IAGP_LOG_ERROR_MESSAGE LogError

// define your fucntion for log error message of IAGP only in debug
//#define IAGP_LOG_DEBUG_ERROR_MESSAGE LogDebugError
