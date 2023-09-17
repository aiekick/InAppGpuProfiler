/*
MIT License

Copyright (c) 2023-2023 Stephane Cuillerdier (aka aiekick)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui.h>
#include <imgui_internal.h>

#include <3rdparty/imgui/backends/imgui_impl_opengl3.h>
#include <3rdparty/imgui/backends/imgui_impl_glfw.h>
#include <InAppGpuProfiler/InAppGpuProfiler.h>
#include <stdio.h>
#include <sstream>
#include <cstdarg>
#include <fstream>
#include <clocale>
#include <string>
#include <glad/glad.h>  // Initialize with gladLoadGL()

// Include glfw3.h after our OpenGL definitions
#include <GLFW/glfw3.h>

#include <glApi/glApi.hpp>

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

static std::string toStr(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char TempBuffer[1024 * 3 + 1] = {};
    const auto w = vsnprintf(TempBuffer, 3072, fmt, args);
    va_end(args);
    if (w) {
        return std::string(TempBuffer, (size_t)w);
    }
    return std::string();
}

glApi::ShaderPtr shader_quad_ptr = nullptr;
glApi::QuadMeshPtr quadMeshPtr = nullptr;
glApi::QuadVfxPtr quadVfxPtr = nullptr;

// Global uniforms
float uniformTimeValue = 0.0f;

bool init_shaders(const float& vSx, const float& vSy) {
    bool res = false;
    quadMeshPtr = glApi::QuadMesh::create();
    if (quadMeshPtr != nullptr) {
        shader_quad_ptr = glApi::Shader::createFromFile("Quad", GL_VERTEX_SHADER, "shaders/quad.vert");
        if (shader_quad_ptr != nullptr) {
            quadVfxPtr = glApi::QuadVfx::create("First Test", shader_quad_ptr, quadMeshPtr, "shaders/shader00.frag", vSx, vSy, 1U);
            if (quadVfxPtr != nullptr) {
                // add unforms
                quadVfxPtr->addUniform(GL_FRAGMENT_SHADER, "uTime", &uniformTimeValue, true);
                // finalize we are ready to rendering it
                quadVfxPtr->finalizeBeforeRendering();
                res = true;
            }
        }    
    }
    if (res) {
        iagp::InAppGpuProfiler::Instance()->puIsActive = true;
    }
    return res;
}

bool resize_shaders(const float& vSx, const float& vSy) {
    if (quadVfxPtr != nullptr) {
        return quadVfxPtr->resize(vSx, vSy);
    }
    return false;
}

void render_shaders() {
    AIGPNewFrame("", "GPU Frame");  // a main Zone is always needed
    if (quadVfxPtr != nullptr) {
        quadVfxPtr->render();
    }
}

void unit_shaders() {
    shader_quad_ptr.reset();
    quadMeshPtr.reset();
    quadVfxPtr.reset();
}

void calc_imgui() {
    ImGui::Begin("Shaders");
    if (quadVfxPtr != nullptr) {
        quadVfxPtr->drawImGuiThumbnail();
    }
    ImGui::End();

    ImGui::Begin("Uniforms");
    if (quadVfxPtr != nullptr) {
        quadVfxPtr->drawUniformWidgets();
    }
    ImGui::End();

    ImGui::Begin("In App Gpu Profiler", nullptr, ImGuiWindowFlags_MenuBar);
    iagp::InAppGpuProfiler::Instance()->Draw();
    ImGui::End();
}

int main(int, char**) {
#ifdef _MSC_VER
    // active memory leak detector
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) return 1;

    // Decide GL+GLSL versions
    const char* glsl_version = "#version 330";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Dear ImGui GLFW+OpenGL3 example", NULL, NULL);
    if (window == NULL) return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);  // Enable vsync

    bool err = gladLoadGL() == 0;
    if (err) {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return 1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.FontAllowUserScaling = true;  // zoom wiht ctrl + mouse wheel
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    ImGui::GetIO().Fonts->AddFontDefault();

    double ratio = 16.0 / 9.0;
    int thumbnail_height = 200;
    int thumbnail_width = (int)(ratio * (double)thumbnail_height);

    int last_display_w = 0, last_display_h = 0;
    if (init_shaders(thumbnail_width, thumbnail_height)) {  // Main loop
        //resize_shaders(thumbnail_width, thumbnail_height);
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);

            if (display_w > 0 && display_h > 0) {
                glfwMakeContextCurrent(window);

                {
                    AIGPNewFrame("", "GPU Frame");  // a main Zone is always needed
                    render_shaders();

                    // Start the Dear ImGui frame
                    ImGui_ImplOpenGL3_NewFrame();
                    ImGui_ImplGlfw_NewFrame();
                    ImGui::NewFrame();

                    // Cpu Zone : prepare
                    calc_imgui();
                    ImGui::Render();

                    // GPU Zone : Rendering
                    glfwMakeContextCurrent(window);
                    {
                        AIGPScoped("", "ImGui::Render");
                        glViewport(0, 0, display_w, display_h);
                        glClearColor(0.3f, 0.3f, 0.3f, 0.3f);
                        glClear(GL_COLOR_BUFFER_BIT);
                        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
                    }
                }

                AIGPCollect;  // collect all measure queries

                glfwSwapBuffers(window);

                uniformTimeValue += ImGui::GetIO().DeltaTime;
            }
        }
    }    

    // Cleanup
    unit_shaders();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
