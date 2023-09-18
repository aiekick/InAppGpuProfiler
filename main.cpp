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
#include <array>
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
glApi::QuadVfxPtr quadVfxPtrs[3][3] = {};

// globals
bool pauseRendering = false;
bool show_shaders = true;
bool show_uniforms = false;
bool show_profiler_details = false;
bool show_profiler_flame_graph = true;
bool use_mipmapping = false;
int32_t thumbnail_width = 200;

// common uniforms
// array is not vector, default value must be set
std::array<int, 1U> uniformFrame = {};
std::array<float, 1U> uniformTime = {};
std::array<float, 3U> uniformResolution = {};

// specific uniforms

glApi::QuadVfxPtr init_shader_00(const float& vSx, const float& vSy) {
    auto res_ptr = glApi::QuadVfx::create("Vfx 00", shader_quad_ptr, quadMeshPtr, "shaders/shader00.frag", vSx, vSy, 1U, use_mipmapping, false);
    assert(res_ptr != nullptr);
    res_ptr->addUniformFloat(GL_FRAGMENT_SHADER, "iTime", uniformTime.data(), uniformTime.size(), false);
    res_ptr->finalizeBeforeRendering();
    return res_ptr;
}

glApi::QuadVfxPtr init_shader_01(const float& vSx, const float& vSy) {
    auto res_ptr = glApi::QuadVfx::create("Vfx 01", shader_quad_ptr, quadMeshPtr, "shaders/shader01.frag", vSx, vSy, 1U, use_mipmapping, false);
    assert(res_ptr != nullptr);
    res_ptr->addUniformFloat(GL_FRAGMENT_SHADER, "iTime", uniformTime.data(), uniformTime.size(), false);
    res_ptr->addUniformFloat(GL_FRAGMENT_SHADER, "iResolution", uniformResolution.data(), uniformResolution.size(), false);
    res_ptr->finalizeBeforeRendering();
    return res_ptr;
}

glApi::QuadVfxPtr init_shader_02(const float& vSx, const float& vSy) {
    auto res_ptr = glApi::QuadVfx::create("Vfx 02", shader_quad_ptr, quadMeshPtr, "shaders/shader02.frag", vSx, vSy, 1U, use_mipmapping, false);
    assert(res_ptr != nullptr);
    res_ptr->addUniformFloat(GL_FRAGMENT_SHADER, "iTime", uniformTime.data(), uniformTime.size(), false);
    res_ptr->addUniformFloat(GL_FRAGMENT_SHADER, "iResolution", uniformResolution.data(), uniformResolution.size(), false);
    res_ptr->finalizeBeforeRendering();
    return res_ptr;
}

// simple multipass
glApi::QuadVfxPtr init_shader_10(const float& vSx, const float& vSy) {
    auto res_ptr = glApi::QuadVfx::create("Vfx 10", shader_quad_ptr, quadMeshPtr, "shaders/shader10.frag", vSx, vSy, 1U, use_mipmapping, true);
    assert(res_ptr != nullptr);
    res_ptr->addUniformFloat(GL_FRAGMENT_SHADER, "iTime", uniformTime.data(), uniformTime.size(), false);
    res_ptr->addUniformInt(GL_FRAGMENT_SHADER, "iFrame", uniformFrame.data(), uniformFrame.size(), false);
    res_ptr->addUniformFloat(GL_FRAGMENT_SHADER, "iResolution", uniformResolution.data(), uniformResolution.size(), false);
    res_ptr->addUniformSampler2D(GL_FRAGMENT_SHADER, "iChannel0", -1, false);
    res_ptr->setUniformPreUploadFunctor([](glApi::FBOPipeLinePtr vFBOPipeLinePtr, glApi::Program::Uniform& vUniform) {
        if (vFBOPipeLinePtr != nullptr) {
            if (vUniform.name == "iChannel0") {
                vUniform.data_s2d = vFBOPipeLinePtr->getBackTextureId(0U);  // get the back buffer at location 0 to use in the front buffer
            }
        }
        });
    res_ptr->finalizeBeforeRendering();
    return res_ptr;
}

glApi::QuadVfxPtr init_shader_11(const float& vSx, const float& vSy) {
    auto res_ptr = glApi::QuadVfx::create("Vfx 11", shader_quad_ptr, quadMeshPtr, "shaders/shader11.frag", vSx, vSy, 1U, use_mipmapping, false);
    assert(res_ptr != nullptr);
    res_ptr->addUniformFloat(GL_FRAGMENT_SHADER, "iTime", uniformTime.data(), uniformTime.size(), false);
    res_ptr->addUniformFloat(GL_FRAGMENT_SHADER, "iResolution", uniformResolution.data(), uniformResolution.size(), false);
    res_ptr->finalizeBeforeRendering();
    return res_ptr;
}

glApi::QuadVfxPtr init_shader_12(const float& vSx, const float& vSy) {
    auto res_ptr = glApi::QuadVfx::create("Vfx 12", shader_quad_ptr, quadMeshPtr, "shaders/shader12.frag", vSx, vSy, 1U, use_mipmapping, false);
    assert(res_ptr != nullptr);
    res_ptr->addUniformFloat(GL_FRAGMENT_SHADER, "iTime", uniformTime.data(), uniformTime.size(), false);
    res_ptr->finalizeBeforeRendering();
    return res_ptr;
}

glApi::QuadVfxPtr init_shader_20(const float& vSx, const float& vSy) {
    auto res_ptr = glApi::QuadVfx::create("Vfx 20", shader_quad_ptr, quadMeshPtr, "shaders/shader20.frag", vSx, vSy, 1U, use_mipmapping, false);
    assert(res_ptr != nullptr);
    res_ptr->addUniformFloat(GL_FRAGMENT_SHADER, "iTime", uniformTime.data(), uniformTime.size(), false);
    res_ptr->finalizeBeforeRendering();
    return res_ptr;
}

glApi::QuadVfxPtr init_shader_21(const float& vSx, const float& vSy) {
    auto res_ptr = glApi::QuadVfx::create("Vfx 21", shader_quad_ptr, quadMeshPtr, "shaders/shader21.frag", vSx, vSy, 1U, use_mipmapping, false);
    assert(res_ptr != nullptr);
    res_ptr->addUniformFloat(GL_FRAGMENT_SHADER, "iTime", uniformTime.data(), uniformTime.size(), false);
    res_ptr->finalizeBeforeRendering();
    return res_ptr;
}

glApi::QuadVfxPtr init_shader_22(const float& vSx, const float& vSy) {
    auto res_ptr = glApi::QuadVfx::create("Vfx 22", shader_quad_ptr, quadMeshPtr, "shaders/shader22.frag", vSx, vSy, 1U, use_mipmapping, false);
    assert(res_ptr != nullptr);
    res_ptr->addUniformFloat(GL_FRAGMENT_SHADER, "iTime", uniformTime.data(), uniformTime.size(), false);
    res_ptr->finalizeBeforeRendering();
    return res_ptr;
}

bool init_shaders(const float& vSx, const float& vSy) {
    bool res = false;
    quadMeshPtr = glApi::QuadMesh::create();
    if (quadMeshPtr != nullptr) {
        shader_quad_ptr = glApi::Shader::createFromFile("Quad", GL_VERTEX_SHADER, "shaders/quad.vert");
        if (shader_quad_ptr != nullptr) {
            quadVfxPtrs[0][0] = init_shader_00(vSx, vSy);
            quadVfxPtrs[0][1] = init_shader_01(vSx, vSy);
            quadVfxPtrs[0][2] = init_shader_02(vSx, vSy);
            quadVfxPtrs[1][0] = init_shader_10(vSx, vSy);
            quadVfxPtrs[1][1] = init_shader_11(vSx, vSy);
            quadVfxPtrs[1][2] = init_shader_12(vSx, vSy);
            quadVfxPtrs[2][0] = init_shader_20(vSx, vSy);
            quadVfxPtrs[2][1] = init_shader_21(vSx, vSy);
            quadVfxPtrs[2][2] = init_shader_22(vSx, vSy);
            res = true;
        }
    }
    if (res) {
        iagp::InAppGpuProfiler::Instance()->puIsActive = true;
    }
    return res;
}

bool resize_shaders(const float& vSx, const float& vSy) {
    bool res = true;
    for (size_t x = 0U; x < 3; ++x) {
        for (size_t y = 0U; y < 3; ++y) {
            assert(quadVfxPtrs[x][y] != nullptr);
            res &= quadVfxPtrs[x][y]->resize(vSx, vSy);
        }
    }
    return res;
}

void render_shaders() {
    if (!pauseRendering) {
        AIGPScoped("render_shaders", "Shaders");
        for (size_t x = 0U; x < 3; ++x) {
            for (size_t y = 0U; y < 3; ++y) {
                assert(quadVfxPtrs[x][y] != nullptr);
                quadVfxPtrs[x][y]->render();
            }
        }
    }
}

void unit_shaders() {
    shader_quad_ptr.reset();
    quadMeshPtr.reset();
    for (size_t x = 0U; x < 3; ++x) {
        for (size_t y = 0U; y < 3; ++y) {
            assert(quadVfxPtrs[x][y] != nullptr);
            quadVfxPtrs[x][y].reset();
        }
    }
}

void calc_imgui() {
    if (ImGui::BeginMainMenuBar()) {
        ImGui::MenuItem("Shaders", nullptr, &show_shaders);
        ImGui::Spacing();
        ImGui::MenuItem("Uniforms", nullptr, &show_uniforms);
        ImGui::Spacing();
        ImGui::MenuItem("Details", nullptr, &show_profiler_details);
        ImGui::Spacing();
        ImGui::MenuItem("Flame Graph", nullptr, &show_profiler_flame_graph);
        ImGui::EndMainMenuBar();
    }

    if (show_shaders) {
        ImGui::Begin("Shaders", nullptr, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_AlwaysAutoResize);
        if (ImGui::BeginMenuBar()) {
            const char* play_pause_label = "Pause";
            if (pauseRendering) {
                play_pause_label = "Play";
            }
            ImGui::Checkbox(play_pause_label, &pauseRendering);

            ImGui::EndMenuBar();
        }
        for (size_t x = 0U; x < 3; ++x) {
            for (size_t y = 0U; y < 3; ++y) {
                assert(quadVfxPtrs[x][y] != nullptr);
                quadVfxPtrs[x][y]->drawImGuiThumbnail();
                if (y != 2U) {
                    ImGui::SameLine();
                }
            }
        }
        ImGui::End();
    }

    if (show_uniforms) {
        ImGui::Begin("Uniforms");
        ImGui::Text("Global Uniforms");
        ImGui::Indent();
        {
            ImGui::DragFloat("Time", &uniformTime[0]);
            ImGui::DragFloat2("Resolution", &uniformResolution[0]);
        }
        ImGui::Unindent();
        ImGui::Text("Program Uniforms");
        for (size_t x = 0U; x < 3; ++x) {
            for (size_t y = 0U; y < 3; ++y) {
                assert(quadVfxPtrs[x][y] != nullptr);
                quadVfxPtrs[x][y]->drawUniformWidgets();
            }
        }
        ImGui::End();
    }

    if (show_profiler_details) {
        ImGui::Begin("In App Gpu Profiler Details", nullptr, ImGuiWindowFlags_MenuBar);
        iagp::InAppGpuProfiler::Instance()->DrawDetails();
        ImGui::End();
    }

    if (show_profiler_flame_graph) {
        ImGui::Begin("In App Gpu Profiler Flame Graph", nullptr, ImGuiWindowFlags_MenuBar);
        iagp::InAppGpuProfiler::Instance()->DrawFlamGraph();
        ImGui::End();
    }
}

int main(int, char**) {
#ifdef _MSC_VER
    // active memory leak detector
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Decide GL+GLSL versions
    const char* glsl_version = "#version 330";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Dear ImGui GLFW+OpenGL3 example", NULL, NULL);
    if (window == NULL)
        return 1;
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
    int thumbnail_height = thumbnail_width;
    int thumbnail_width = (int)(ratio * (double)thumbnail_height);
    uniformResolution[0] = (float)thumbnail_width;
    uniformResolution[1] = (float)thumbnail_height;

    int last_display_w = 0, last_display_h = 0;
    if (init_shaders(thumbnail_width, thumbnail_height)) {  // Main loop
        // resize_shaders(thumbnail_width, thumbnail_height);
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);

            if (display_w > 0 && display_h > 0) {
                glfwMakeContextCurrent(window);

                {
                    AIGPNewFrame("GPU Frame", "GPU Frame");  // a main Zone is always needed
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
                        AIGPScoped("ImGui", "Render");
                        glViewport(0, 0, display_w, display_h);
                        glClearColor(0.3f, 0.3f, 0.3f, 0.3f);
                        glClear(GL_COLOR_BUFFER_BIT);
                        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
                    }
                }

                AIGPCollect;  // collect all measure queries

                glfwSwapBuffers(window);

                uniformTime[0] += ImGui::GetIO().DeltaTime;
                ++uniformFrame[0];
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
