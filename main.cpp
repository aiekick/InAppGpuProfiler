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

#define LABEL_MAX_DISPLAY_WIDTH 150.0f

static bool SliderFloatDefault(const char* label, float* v, float v_min, float v_max, float v_default) {
    assert(v != nullptr);
    ImGui::PushID(label);
    ImGui::Text("%s", label);
    ImGui::SameLine(LABEL_MAX_DISPLAY_WIDTH);
    if (ImGui::Button("R")) {
        *v = v_default;
    }
    ImGui::SameLine();
    bool res = ImGui::SliderFloat("##SliderFloat", v, v_min, v_max);
    ImGui::PopID();
    return res;
}
static bool SliderIntDefault(const char* label, int32_t* v, int32_t v_min, int32_t v_max, int32_t v_default) {
    assert(v != nullptr);
    ImGui::PushID(label);
    ImGui::Text("%s", label);
    ImGui::SameLine(LABEL_MAX_DISPLAY_WIDTH);
    if (ImGui::Button("R")) {
        *v = v_default;
    }
    ImGui::SameLine();
    bool res = ImGui::SliderInt("##SliderInt", v, v_min, v_max);
    ImGui::PopID();
    return res;
}
static bool SliderUIntDefault(const char* label, uint32_t* v, uint32_t v_min, uint32_t v_max, uint32_t v_default) {
    assert(v != nullptr);
    ImGui::PushID(label);
    ImGui::Text("%s", label);
    ImGui::SameLine(LABEL_MAX_DISPLAY_WIDTH);
    if (ImGui::Button("R")) {
        *v = v_default;
    }
    ImGui::SameLine();
    bool res = ImGui::SliderScalar("##SliderInt", ImGuiDataType_U32, v, &v_min, &v_max);
    ImGui::PopID();
    return res;
}
static bool Color3Default(const char* label, float* v, float v_default[3]) {
    assert(v != nullptr);
    ImGui::PushID(label);
    ImGui::Text("%s", label);
    ImGui::SameLine(LABEL_MAX_DISPLAY_WIDTH);
    if (ImGui::Button("R")) {
        memcpy(v, v_default, sizeof(float) * 3U);
    }
    ImGui::SameLine();
    bool res = ImGui::ColorEdit3("##ColorEdit3", v);
    ImGui::PopID();
    return res;
}
static bool CheckBoxDefault(const char* label, bool* v, bool v_default) {
    assert(v != nullptr);
    ImGui::PushID(label);
    ImGui::Text("%s", label);
    ImGui::SameLine(LABEL_MAX_DISPLAY_WIDTH);
    if (ImGui::Button("R")) {
       *v = v_default;
    }
    ImGui::SameLine();
    bool res = ImGui::Checkbox("##Checkbox", v);
    ImGui::PopID();
    return res;
}

glApi::ShaderPtr shader_quad_ptr = nullptr;
glApi::QuadMeshPtr quadMeshPtr = nullptr;
glApi::QuadVfxPtr quadVfxPtrs[3][3] = {};
glApi::QuadVfxWeak currentQuadVFX; // current to show in shader view

    // globals
bool pauseRendering = false;
bool show_imgui = false;
bool show_shaders = true;
bool show_uniforms = false;
bool show_controls = false;
bool show_shader_view = false;
bool show_profiler_details = false;
bool show_profiler_flame_graph = true;
int32_t thumbnail_height = 200;

// common uniforms
// array is not vector, default value must be set
std::array<int, 1U> uniformFrame = {};
std::array<float, 1U> uniformTime = {};
std::array<float, 3U> uniformResolution = {};

glApi::QuadVfxPtr init_shader_00(const float& vSx, const float& vSy) {
    auto res_ptr = glApi::QuadVfx::create("Vfx 00", shader_quad_ptr, quadMeshPtr, "shaders/shader00.frag", vSx, vSy, 1U, false, false);
    assert(res_ptr != nullptr);
    res_ptr->addUniformFloat(GL_FRAGMENT_SHADER, "iTime", uniformTime.data(), uniformTime.size(), false, nullptr);
    res_ptr->addUniformFloat(GL_FRAGMENT_SHADER, "iResolution", uniformResolution.data(), uniformResolution.size(), false, nullptr);
    res_ptr->finalizeBeforeRendering();
    return res_ptr;
}

glApi::QuadVfxPtr init_shader_01(const float& vSx, const float& vSy) {
    auto res_ptr = glApi::QuadVfx::create("Vfx 01", shader_quad_ptr, quadMeshPtr, "shaders/shader01.frag", vSx, vSy, 1U, false, false);
    assert(res_ptr != nullptr);
    res_ptr->addUniformFloat(GL_FRAGMENT_SHADER, "iTime", uniformTime.data(), uniformTime.size(), false, nullptr);
    res_ptr->addUniformFloat(GL_FRAGMENT_SHADER, "iResolution", uniformResolution.data(), uniformResolution.size(), false, nullptr);
    res_ptr->finalizeBeforeRendering();
    return res_ptr;
}

glApi::QuadVfxPtr init_shader_02(const float& vSx, const float& vSy) {
    auto res_ptr = glApi::QuadVfx::create("Vfx 02", shader_quad_ptr, quadMeshPtr, "shaders/shader02.frag", vSx, vSy, 1U, false, false);
    assert(res_ptr != nullptr);
    res_ptr->addUniformFloat(GL_FRAGMENT_SHADER, "iTime", uniformTime.data(), uniformTime.size(), false, nullptr);
    res_ptr->addUniformFloat(GL_FRAGMENT_SHADER, "iResolution", uniformResolution.data(), uniformResolution.size(), false, nullptr);
    res_ptr->finalizeBeforeRendering();
    return res_ptr;
}

// simple multipass
glApi::QuadVfxPtr init_shader_10(const float& vSx, const float& vSy) {
    auto res_ptr = glApi::QuadVfx::create("Vfx 10", shader_quad_ptr, quadMeshPtr, "shaders/shader10.frag", vSx, vSy, 1U, false, true);
    assert(res_ptr != nullptr);
    res_ptr->addUniformFloat(GL_FRAGMENT_SHADER, "iTime", uniformTime.data(), uniformTime.size(), false, nullptr);
    res_ptr->addUniformInt(GL_FRAGMENT_SHADER, "iFrame", uniformFrame.data(), uniformFrame.size(), false, nullptr);
    res_ptr->addUniformFloat(GL_FRAGMENT_SHADER, "iResolution", uniformResolution.data(), uniformResolution.size(), false, nullptr);
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
    auto res_ptr = glApi::QuadVfx::create("Vfx 11", shader_quad_ptr, quadMeshPtr, "shaders/shader11.frag", vSx, vSy, 1U, false, false);
    assert(res_ptr != nullptr);

    res_ptr->addUniformFloat(GL_FRAGMENT_SHADER, "iTime", uniformTime.data(), uniformTime.size(), false, nullptr);
    res_ptr->addUniformFloat(GL_FRAGMENT_SHADER, "iResolution", uniformResolution.data(), uniformResolution.size(), false, nullptr);

    // uniform vec2(0.0:1.0:0.924,0.0) _c;
    static std::array<float, 2U> _c = {0.924f, 0.0f};
    res_ptr->addUniformFloat(                                  //
        GL_FRAGMENT_SHADER, "_c", _c.data(), _c.size(), true,  //
        [](glApi::Program::Uniform& vUniform) {                //
            SliderFloatDefault("_c x", &vUniform.datas_f[0], 0.0f, 1.0f, 0.924f);
            SliderFloatDefault("_c y", &vUniform.datas_f[1], 0.0f, 1.0f, 0.0f);
        });

    // uniform int(0:200:100) _niter;
    static std::array<int, 1U> _niter = {100};
    res_ptr->addUniformInt(                                                //
        GL_FRAGMENT_SHADER, "_niter", _niter.data(), _niter.size(), true,  //
        [](glApi::Program::Uniform& vUniform) {                            //
            SliderIntDefault("_niter", &vUniform.datas_i[0], 0, 200, 100);
        });

    // uniform float(0.0:1.0:0.25912) _k;
    static std::array<float, 1U> _k = {0.25912f};
    res_ptr->addUniformFloat(                                  //
        GL_FRAGMENT_SHADER, "_k", _k.data(), _k.size(), true,  //
        [](glApi::Program::Uniform& vUniform) {                //
            SliderFloatDefault("_k", &vUniform.datas_f[0], 0.0f, 1.0f, 0.25912f);
        });

    // uniform float(0.0:5.0:2.2) _scale;
    static std::array<float, 1U> _scale = {2.2f};
    res_ptr->addUniformFloat(                                              //
        GL_FRAGMENT_SHADER, "_scale", _scale.data(), _scale.size(), true,  //
        [](glApi::Program::Uniform& vUniform) {                            //
            SliderFloatDefault("_scale", &vUniform.datas_f[0], 0.0f, 5.0f, 2.0f);
        });

    // uniform float(0.0:0.5:0.03) _limit;
    static std::array<float, 1U> _limit = {0.03f};
    res_ptr->addUniformFloat(                                              //
        GL_FRAGMENT_SHADER, "_limit", _limit.data(), _limit.size(), true,  //
        [](glApi::Program::Uniform& vUniform) {                            //
            SliderFloatDefault("_limit", &vUniform.datas_f[0], 0.0f, 0.5f, 0.03f);
        });

    // uniform float(0.0:100.0:8.0) _dist;
    static std::array<float, 1U> _dist = {8.0f};
    res_ptr->addUniformFloat(                                           //
        GL_FRAGMENT_SHADER, "_dist", _dist.data(), _dist.size(), true,  //
        [](glApi::Program::Uniform& vUniform) {                         //
            SliderFloatDefault("_dist", &vUniform.datas_f[0], 0.0f, 100.0f, 8.0f);
        });

    // uniform vec3(color:1,0,1) _color;
    static std::array<float, 3U> _color_default = {1.0f, 0.0f, 1.0f};
    static std::array<float, 3U> _color = {1.0f, 0.0f, 1.0f};
    res_ptr->addUniformFloat(                                              //
        GL_FRAGMENT_SHADER, "_color", _color.data(), _color.size(), true,  //
        [](glApi::Program::Uniform& vUniform) {                            //
            if (vUniform.channels == 3U) {
                Color3Default("_color", &vUniform.datas_f[0], _color_default.data());
            }
        });

    // uniform vec2(0:5:3,1.51) _colorVar;
    static std::array<float, 2U> _colorVar = {3.0f, 1.51f};
    res_ptr->addUniformFloat(                                                       //
        GL_FRAGMENT_SHADER, "_colorVar", _colorVar.data(), _colorVar.size(), true,  //
        [](glApi::Program::Uniform& vUniform) {                                     //
            SliderFloatDefault("_colorVar x", &vUniform.datas_f[0], 0.0f, 5.0f, 3.0f);
            SliderFloatDefault("_colorVar y", &vUniform.datas_f[1], 0.0f, 5.0f, 1.51f);
        });

    res_ptr->finalizeBeforeRendering();
    return res_ptr;
}

glApi::QuadVfxPtr init_shader_12(const float& vSx, const float& vSy) {
    auto res_ptr = glApi::QuadVfx::create("Vfx 12", shader_quad_ptr, quadMeshPtr, "shaders/shader12.frag", vSx, vSy, 1U, false, false);
    assert(res_ptr != nullptr);
    res_ptr->addUniformFloat(GL_FRAGMENT_SHADER, "iTime", uniformTime.data(), uniformTime.size(), false, nullptr);
    res_ptr->addUniformFloat(GL_FRAGMENT_SHADER, "iResolution", uniformResolution.data(), uniformResolution.size(), false, nullptr);
    res_ptr->finalizeBeforeRendering();
    return res_ptr;
}

glApi::QuadVfxPtr init_shader_20(const float& vSx, const float& vSy) {
    auto res_ptr = glApi::QuadVfx::create("Vfx 20", shader_quad_ptr, quadMeshPtr, "shaders/shader20.frag", vSx, vSy, 1U, false, false);
    assert(res_ptr != nullptr);
    res_ptr->addUniformFloat(GL_FRAGMENT_SHADER, "iTime", uniformTime.data(), uniformTime.size(), false, nullptr);
    res_ptr->addUniformFloat(GL_FRAGMENT_SHADER, "iResolution", uniformResolution.data(), uniformResolution.size(), false, nullptr);
    res_ptr->finalizeBeforeRendering();
    return res_ptr;
}

glApi::QuadVfxPtr init_shader_21(const float& vSx, const float& vSy) {
    auto res_ptr = glApi::QuadVfx::create("Vfx 21", shader_quad_ptr, quadMeshPtr, "shaders/shader21.frag", vSx, vSy, 1U, false, false);
    assert(res_ptr != nullptr);
    res_ptr->addUniformFloat(GL_FRAGMENT_SHADER, "iTime", uniformTime.data(), uniformTime.size(), false, nullptr);
    res_ptr->addUniformFloat(GL_FRAGMENT_SHADER, "iResolution", uniformResolution.data(), uniformResolution.size(), false, nullptr);
    res_ptr->finalizeBeforeRendering();
    return res_ptr;
}

glApi::QuadVfxPtr init_shader_22(const float& vSx, const float& vSy) {
    static std::array<float, 3U> uniformBiggerResolution = {vSx * 2.0f, vSy * 2.0f, 0.0f}; // resolution * 2.0 for better details
    auto res_ptr = glApi::QuadVfx::create(                                //
        "Vfx 22", shader_quad_ptr, quadMeshPtr, "shaders/shader22.frag",  //
        uniformBiggerResolution[0], uniformBiggerResolution[1], 2U, true, true);
    assert(res_ptr != nullptr);
    res_ptr->addUniformFloat(GL_FRAGMENT_SHADER, "iTime", uniformTime.data(), uniformTime.size(), false, nullptr);
    res_ptr->addUniformInt(GL_FRAGMENT_SHADER, "iFrame", uniformFrame.data(), uniformFrame.size(), false, nullptr);
    res_ptr->addUniformFloat(                                                                                     //
        GL_FRAGMENT_SHADER, "iResolution", uniformBiggerResolution.data(), uniformBiggerResolution.size(), true,  //
        [](glApi::Program::Uniform& vUniform) {
            ImGui::Text("iResolution x");
            ImGui::SameLine(LABEL_MAX_DISPLAY_WIDTH);
            ImGui::Text("%.1f", uniformBiggerResolution[0]);
            ImGui::Text("iResolution y");
            ImGui::SameLine(LABEL_MAX_DISPLAY_WIDTH);
            ImGui::Text("%.1f", uniformBiggerResolution[1]);
        });
    res_ptr->addUniformSampler2D(GL_FRAGMENT_SHADER, "iChannel0", -1, false);
    res_ptr->setUniformPreUploadFunctor([](glApi::FBOPipeLinePtr vFBOPipeLinePtr, glApi::Program::Uniform& vUniform) {
        if (vFBOPipeLinePtr != nullptr) {
            if (vUniform.name == "iChannel0") {
                vUniform.data_s2d = vFBOPipeLinePtr->getBackTextureId(1U);  // get the back buffer at location 1 to use in the front buffer
            }
        }
    });
    res_ptr->finalizeBeforeRendering();
    res_ptr->setRenderingIterations(4U);  // speed x4
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

void clear_buffers(const std::array<float, 4U>& vColor) {
    AIGPScoped("clear_buffers", "BHuffer");
    for (size_t x = 0U; x < 3; ++x) {
        for (size_t y = 0U; y < 3; ++y) {
            assert(quadVfxPtrs[x][y] != nullptr);
            quadVfxPtrs[x][y]->clearBuffers(vColor);
        }
    }
}

void render_shaders(const bool& vForce = false) {
    if (!pauseRendering || vForce) {
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

void shader_view() {
    if (!currentQuadVFX.expired()) {
        auto ptr = currentQuadVFX.lock();
        if (ptr != nullptr) {
            const auto& size = ImGui::GetContentRegionAvail();
            const auto& vfx_size = ptr->getSize();
            const auto& tex_id = ptr->getTextureId();
            float scale_inv = (float)thumbnail_height / vfx_size[1];
            ImGui::Image((ImTextureID)tex_id, size, ImVec2(0, scale_inv), ImVec2(scale_inv, 0));
        }
    }
}

void calc_imgui() {
    if (ImGui::BeginMainMenuBar()) {
        auto& io = ImGui::GetIO();
        ImGui::Text("Dear ImgGui %s | average %.3f ms/frame (%.1f FPS)", IMGUI_VERSION, 1000.0f / io.Framerate, io.Framerate);
        ImGui::Spacing();
        ImGui::MenuItem("ImGui", nullptr, &show_imgui);
        ImGui::Spacing();
        ImGui::MenuItem("Shaders", nullptr, &show_shaders);
        ImGui::Spacing();
        ImGui::MenuItem("Uniforms", nullptr, &show_uniforms);
        ImGui::Spacing();
        ImGui::MenuItem("Controls", nullptr, &show_controls);
        ImGui::Spacing();
        ImGui::MenuItem("Shader View", nullptr, &show_shader_view);
        ImGui::Spacing();
        ImGui::MenuItem("Details", nullptr, &show_profiler_details);
        ImGui::Spacing();
        ImGui::MenuItem("Flame Graph", nullptr, &show_profiler_flame_graph);
        ImGui::Spacing();
        if (ImGui::Button("Clear Buffers")) {
            std::array<float, 4U> col = {};
            clear_buffers(col);
            render_shaders(true); // one update after
        }
        ImGui::EndMainMenuBar();
    }

    if (show_imgui) {
        ImGui::ShowDemoWindow(&show_imgui);
    }

    if (show_shaders) {
        ImGui::Begin("Shaders", &show_shaders, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_AlwaysAutoResize);
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
                auto ptr = quadVfxPtrs[x][y];
                assert(ptr != nullptr);
                const auto& vfx_size = ptr->getSize();
                float scale_inv = (float)thumbnail_height / vfx_size[1];
                auto tex_id = ptr->getTextureId();
                if (tex_id > 0U) {
                    if (ImGui::ImageButton(ptr->getLabelName(), (ImTextureID)tex_id, ImVec2(uniformResolution[0], uniformResolution[1]),
                                           ImVec2(0, scale_inv), ImVec2(scale_inv, 0))) {
                        if (currentQuadVFX.lock() == ptr) {
                            show_shader_view = !show_shader_view;
                        } else {
                            show_shader_view = true;
                        }
                        if (show_shader_view) {
                            currentQuadVFX = ptr;
                        } else {
                            currentQuadVFX.reset();
                        }
                    }
                }
                if (y != 2U) {
                    ImGui::SameLine();
                }
            }
        }
        ImGui::End();
    }

    if (show_uniforms) {
        ImGui::Begin("Uniforms", &show_uniforms);
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

    if (show_controls) {
        ImGui::Begin("Controls", &show_controls);
        for (size_t x = 0U; x < 3; ++x) {
            for (size_t y = 0U; y < 3; ++y) {
                auto ptr = quadVfxPtrs[x][y];
                assert(ptr != nullptr);
                ImGui::Text("Vfx %u%u", (uint32_t)x, (uint32_t)y);
                ImGui::PushID(ptr.get());
                ImGui::Indent();
                auto& rendering_pause = ptr->getRenderingPauseRef();
                const char* pause_label = "Pause";
                if (rendering_pause) {
                    pause_label = "Play";
                }
                CheckBoxDefault(pause_label, &rendering_pause, false);
                SliderUIntDefault("Iterations", &ptr->getRenderingIterationsRef(), 1U, 10U, 1U);
                ImGui::Unindent();
                ImGui::PopID();
            }
        }
        ImGui::End();
    }

    if (show_shader_view) {
        ImGui::Begin("Shader View", &show_shader_view, ImGuiWindowFlags_MenuBar);
        shader_view();
        ImGui::End();
    }

    if (show_profiler_details) {
        ImGui::Begin("In App Gpu Profiler Details", &show_profiler_details, ImGuiWindowFlags_MenuBar);
        iagp::InAppGpuProfiler::Instance()->DrawDetails();
        ImGui::End();
    }

    if (show_profiler_flame_graph) {
        ImGui::Begin("In App Gpu Profiler Flame Graph", &show_profiler_flame_graph, ImGuiWindowFlags_MenuBar);
        iagp::InAppGpuProfiler::Instance()->DrawFlamGraph();
        ImGui::End();

        for (auto& queryZone : iagp::InAppGpuQueryZone::sTabbedQueryZones) {
            if (!queryZone.expired()) {
                auto ptr = queryZone.lock();
                if (ptr != nullptr) {
                    bool opened = true;
                    ImGui::Begin(ptr->puName.c_str(), &opened, ImGuiWindowFlags_MenuBar);
                    ptr->DrawFlamGraph(iagp::InAppGpuProfiler::Instance()->GetGraphTypeRef());
                    ImGui::End();
                    if (!opened) {
                        // we release the weak
                        queryZone.reset();
                        // unfotunatly we dont removed expired pointer
                        // so with time the for loop will grow up
                        // but its fast so maybe not a big problem
                    }
                }
            }
        }
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
    int thumbnail_width = (int)(ratio * (double)thumbnail_height);
    uniformResolution[0] = (float)thumbnail_width;
    uniformResolution[1] = (float)thumbnail_height;

    int last_display_w = 0, last_display_h = 0;
    if (init_shaders(thumbnail_width, thumbnail_height)) {  // Main loop
        // resize_shaders(thumbnail_width, thumbnail_height);
        while (!glfwWindowShouldClose(window)) {
            {
                AIGPNewFrame("GPU Frame", "GPU Frame");  // a main Zone is always needed
                glfwPollEvents();

                int display_w, display_h;
                glfwGetFramebufferSize(window, &display_w, &display_h);

                if (display_w > 0 && display_h > 0) {
                    glfwMakeContextCurrent(window);

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
                        {
                            AIGPScoped("Opengl", "glViewport");
                            glViewport(0, 0, display_w, display_h);
                        }

                        {
                            AIGPScoped("Opengl", "glClearColor");
                            glClearColor(0.3f, 0.3f, 0.3f, 0.3f);
                        }

                        {
                            AIGPScoped("Opengl", "glClear");
                            glClear(GL_COLOR_BUFFER_BIT);
                        }

                        {
                            AIGPScoped("ImGui", "RenderDrawData");
                            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
                        }
                    }

                    {
                        AIGPScoped("Opengl", "glfwSwapBuffers");
                        glfwSwapBuffers(window);
                    }
                }
            }

            AIGPCollect;  // collect all measure queries out of Main Frame

            // globals uniforms
            uniformTime[0] += ImGui::GetIO().DeltaTime;
            ++uniformFrame[0];
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
