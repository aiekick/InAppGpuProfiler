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

struct ProgramDatas {
    glApi::ShaderPtr vertShaderPtr = nullptr;
    glApi::ShaderPtr fragShaderPtr = nullptr;
    glApi::ProgramPtr programPtr = nullptr;
};

glApi::ShaderPtr shader_quad_ptr = nullptr;
ProgramDatas programs[3][3];

bool init_shaders() {
    bool res = false;
    shader_quad_ptr = glApi::Shader::createFromFile("Quad", GL_VERTEX_SHADER, "shaders/quad.vert");
    if (shader_quad_ptr != nullptr) {
        res = true;
        for (size_t px = 0U; px < 3U; ++px) {
            for (size_t py = 0U; py < 3U; ++py) {
                auto& progDatas = programs[px][py];
                progDatas.vertShaderPtr = shader_quad_ptr;
                const auto frag_name = toStr("Frag%u%u", (uint32_t)px, (uint32_t)py);
                const auto shader_name = toStr("shaders/shader%u%u", (uint32_t)px, (uint32_t)py);
                const auto program_name = toStr("shaders/shader%u%u", (uint32_t)px, (uint32_t)py);
                progDatas.fragShaderPtr = glApi::Shader::createFromFile(frag_name, GL_FRAGMENT_SHADER, shader_name);
                if (progDatas.fragShaderPtr != nullptr) {
                    progDatas.programPtr = glApi::Program::create(program_name);
                    if (progDatas.programPtr != nullptr) {
                        if (progDatas.programPtr->addShader(progDatas.vertShaderPtr)) {
                            if (progDatas.programPtr->addShader(progDatas.fragShaderPtr)) {
                                res &= progDatas.programPtr->link();
                            }
                        }
                    }
                }
            }
        }
    }
    return res;
}

void render_shaders() {

}

void calc_imgui() {
    ImGui::Begin("Shaders");
    {

    }
    ImGui::End();

    ImGui::Begin("Profiler");
    {
        
    }
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
#if APPLE
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

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

    if (init_shaders()) {// Main loop
        while (!glfwWindowShouldClose(window)) {
            // Poll and handle events (inputs, window resize, etc.)
            // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
            // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
            // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
            // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
            glfwPollEvents();

            int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);

            render_shaders();

            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            calc_imgui();

            // Cpu Zone : prepare
            ImGui::Render();

            // GPU Zone : Rendering
            glfwMakeContextCurrent(window);
            glViewport(0, 0, display_w, display_h);
            glClearColor(0.3f, 0.3f, 0.3f, 0.3f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(window);
        }
    }
    

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
