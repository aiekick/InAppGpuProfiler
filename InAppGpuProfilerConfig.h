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
// #define GPU_CONTEXT void*
// #define GET_CURRENT_CONTEXT
// #define SET_CURRENT_CONTEXT SetCurrentContext

////////////////////////////////////////////////////////////////
// OPTIONNAL ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

// the max level of recursion for profiler queries
//#define RECURSIVE_LEVELS_COUNT 20U

// the mean average level 
// all the values will be smoothed on 60 frames (1s of 60fps diosplay)
//#define MEAN_AVERAGE_LEVELS_COUNT 60U

//the minimal size of imgui sub window, when you openif by click right on a progiler bar
//#define SUB_AIGP_WINDOW_MIN_SIZE ImVec2(300, 100)

// the imgui button to use in IAGP
//#define IAGP_IMGUI_BUTTON ImGui::Button

// the Imgui Play/Pause button to use
//#define IMGUI_PLAY_PAUSE_BUTTON

// define your fucntion for log error message of IAGP
//#define LOG_ERROR_MESSAGE LogError

// define your fucntion for log error message of IAGP only in debug
//#define LOG_DEBUG_ERROR_MESSAGE LogDebugError
