/*
MIT License

Copyright (c) 2021-2023 Stephane Cuillerdier (aka aiekick)

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

// This is an independent m_oject of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "InAppGpuProfiler.h"

#include <cstdarg> /* va_list, va_start, va_arg, va_end */
#include <cmath>

#ifdef _MSC_VER
#include <Windows.h>
#define DEBUG_BREAK          \
    if (IsDebuggerPresent()) \
    __debugbreak()
#else
#define DEBUG_BREAK
#endif

#ifndef SUB_AIGP_WINDOW_MIN_SIZE
#define SUB_AIGP_WINDOW_MIN_SIZE ImVec2(300, 100)
#endif // SUB_AIGP_WINDOW_MIN_SIZE

#ifndef GPU_CONTEXT
#define GPU_CONTEXT void*
#endif  // GPU_CONTEXT

#ifndef GET_CURRENT_CONTEXT
static GPU_CONTEXT GetCurrentContext() {
    DEBUG_BREAK;  // you need to create your own function for get the opengl context
    return nullptr;
}
#define GET_CURRENT_CONTEXT GetCurrentContext
#endif  // GET_CURRENT_CONTEXT

#ifndef SET_CURRENT_CONTEXT
static void SetCurrentContext(GPU_CONTEXT vContextPtr) {
    DEBUG_BREAK;  // you need to create your own function for get the opengl context
}
#define SET_CURRENT_CONTEXT SetCurrentContext
#endif  // GET_CURRENT_CONTEXT

#ifndef LOG_ERROR_MESSAGE
static void LogError(const char* fmt, ...) {
    DEBUG_BREAK;  // you need to define your own function for get error messages
}
#define LOG_ERROR_MESSAGE LogError
#endif  // LOG_ERROR_MESSAGE

#ifndef LOG_DEBUG_ERROR_MESSAGE
static void LogDebugError(const char* fmt, ...) {
    DEBUG_BREAK;  // you need to define your own function for get error messages in debug
}
#define LOG_DEBUG_ERROR_MESSAGE LogDebugError
#endif  // LOG_DEBUG_ERROR_MESSAGE

#ifndef IAGP_IMGUI_BUTTON
#define IAGP_IMGUI_BUTTON ImGui ::Button
#endif  // IMGUI_BUTTON

#ifndef IMGUI_PLAY_PAUSE_BUTTON
static bool PlayPauseButton(bool& vPlayPause) {
    bool res = false;
    const char* play_pause_label = "Play";
    if (vPlayPause) {
        play_pause_label = "Pause";
    }
    if (ImGui::Button(play_pause_label)) {
        vPlayPause = !vPlayPause;
        res = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Play/Pause Profiling");
    }
    return res;
}
#define IMGUI_PLAY_PAUSE_BUTTON PlayPauseButton
#endif  // LOG_DEBUG_ERROR_MESSAGE

namespace iagp {

void checkGLErrors(const char* vFile, const char* vFunc, const int& vLine) {
#ifdef _DEBUG
    const GLenum err(glGetError());
    if (err != GL_NO_ERROR) {
        std::string error;
        switch (err) {
            case GL_INVALID_OPERATION: error = "INVALID_OPERATION"; break;
            case GL_INVALID_ENUM: error = "INVALID_ENUM"; break;
            case GL_INVALID_VALUE: error = "INVALID_VALUE"; break;
            case GL_OUT_OF_MEMORY: error = "OUT_OF_MEMORY"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
            case GL_STACK_UNDERFLOW: error = "GL_STACK_UNDERFLOW"; break;
            case GL_STACK_OVERFLOW: error = "GL_STACK_OVERFLOW"; break;
        }
        printf("[%s][%s][%i] GL Errors : %s\n", vFile, vFunc, vLine, error.c_str());
        //DEBUG_BREAK;
    }
#endif
}

#define CheckGLErrors checkGLErrors(__FILE__, __FUNCTION__, __LINE__)

// contrast from 1 to 21
// https://www.w3.org/TR/WCAG20/#relativeluminancedef
static float CalcContrastRatio(const ImU32& backgroundColor, const ImU32& foreGroundColor) {
    const float sa0 = (float)((backgroundColor >> IM_COL32_A_SHIFT) & 0xFF);
    const float sa1 = (float)((foreGroundColor >> IM_COL32_A_SHIFT) & 0xFF);
    static float sr = 0.2126f / 255.0f;
    static float sg = 0.7152f / 255.0f;
    static float sb = 0.0722f / 255.0f;
    const float contrastRatio =
        (sr * sa0 * ((backgroundColor >> IM_COL32_R_SHIFT) & 0xFF) + sg * sa0 * ((backgroundColor >> IM_COL32_G_SHIFT) & 0xFF) +
         sb * sa0 * ((backgroundColor >> IM_COL32_B_SHIFT) & 0xFF) + 0.05f) /
        (sr * sa1 * ((foreGroundColor >> IM_COL32_R_SHIFT) & 0xFF) + sg * sa1 * ((foreGroundColor >> IM_COL32_G_SHIFT) & 0xFF) +
         sb * sa1 * ((foreGroundColor >> IM_COL32_B_SHIFT) & 0xFF) + 0.05f);
    if (contrastRatio < 1.0f)
        return 1.0f / contrastRatio;
    return contrastRatio;
}

static bool PushStyleColorWithContrast(const ImU32& backGroundColor, const ImGuiCol& foreGroundColor, const ImVec4& invertedColor,
                                       const float& maxContrastRatio) {
    const float contrastRatio = CalcContrastRatio(backGroundColor, ImGui::GetColorU32(foreGroundColor));
    if (contrastRatio < maxContrastRatio) {
        ImGui::PushStyleColor(foreGroundColor, invertedColor);
        return true;
    }
    return false;
}

static std::string toStr(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char TempBuffer[1024 * 3 + 1];
    const int w = vsnprintf(TempBuffer, 3072, fmt, args);
    va_end(args);
    if (w) {
        return std::string(TempBuffer, (size_t)w);
    }
    return std::string();
}

////////////////////////////////////////////////////////////
/////////////////////// QUERY ZONE /////////////////////////
////////////////////////////////////////////////////////////

GLuint InAppGpuQueryZone::sMaxDepthToOpen = 100U;  // the max by default
bool InAppGpuQueryZone::sShowLeafMode = false;
float InAppGpuQueryZone::sContrastRatio = 4.3f;
bool InAppGpuQueryZone::sActivateLogger = false;
std::vector<IAGPQueryZoneWeak> InAppGpuQueryZone::sTabbedQueryZones = {};
IAGPQueryZonePtr InAppGpuQueryZone::create(GPU_CONTEXT vContext, const std::string& vName, const std::string& vSectionName, const bool& vIsRoot) {
    auto res = std::make_shared<InAppGpuQueryZone>(vContext, vName, vSectionName, vIsRoot);
    res->m_This = res;
    return res;
}
InAppGpuQueryZone::circularSettings InAppGpuQueryZone::sCircularSettings;

InAppGpuQueryZone::InAppGpuQueryZone(GPU_CONTEXT vContext, const std::string& vName, const std::string& vSectionName, const bool& vIsRoot)
    : m_Context(vContext), name(vName), m_IsRoot(vIsRoot), m_SectionName(vSectionName) {
    m_StartFrameId = 0;
    m_EndFrameId = 0;
    m_StartTimeStamp = 0;
    m_EndTimeStamp = 0;
    m_ElapsedTime = 0.0;
    depth = InAppGpuScopedZone::sCurrentDepth;
    imGuiLabel = vName + "##InAppGpuQueryZone_" + std::to_string((intptr_t)this);

    SET_CURRENT_CONTEXT(m_Context);
    CheckGLErrors;
    glGenQueries(2, ids);
    CheckGLErrors;
}

InAppGpuQueryZone::~InAppGpuQueryZone() {
    SET_CURRENT_CONTEXT(m_Context);
    CheckGLErrors;
    glDeleteQueries(2, ids);
    CheckGLErrors;

    name.clear();
    m_StartFrameId = 0;
    m_EndFrameId = 0;
    m_StartTimeStamp = 0;
    m_EndTimeStamp = 0;
    m_ElapsedTime = 0.0;
    zonesOrdered.clear();
    zonesDico.clear();
}

void InAppGpuQueryZone::Clear() {
    m_StartFrameId = 0;
    m_EndFrameId = 0;
    m_StartTimeStamp = 0;
    m_EndTimeStamp = 0;
    m_ElapsedTime = 0.0;
}

void InAppGpuQueryZone::SetStartTimeStamp(const GLuint64& vValue) {
    m_StartTimeStamp = vValue;
    m_StartFrameId++;
}

void InAppGpuQueryZone::SetEndTimeStamp(const GLuint64& vValue) {
    m_EndTimeStamp = vValue;
    m_EndFrameId++;

#ifdef DEBUG_MODE_LOGGING
    DEBUG_MODE_LOGGING("%*s end id retrieved : %u", depth, "", ids[1]);
#endif
    // start computation of elapsed time
    // no needed after
    // will be used for Graph and labels
    // so DrawMetricGraph must be the first
    ComputeElapsedTime();

    if (InAppGpuQueryZone::sActivateLogger && zonesOrdered.empty())  // only the leafs
    {
        /*double v = (double)vValue / 1e9;
        LogVarLightInfo("<profiler section=\"%s\" epoch_time=\"%f\" name=\"%s\" render_time_ms=\"%f\">",
            m_SectionName.c_str(), v, name.c_str(), m_ElapsedTime);*/
    }
}

void InAppGpuQueryZone::ComputeElapsedTime() {
    // we take the last frame
    if (m_StartFrameId == m_EndFrameId) {
        m_AverageStartValue.AddValue(m_StartTimeStamp);  // ns to ms
        m_AverageEndValue.AddValue(m_EndTimeStamp);      // ns to ms
        m_StartTime = (double)(m_AverageStartValue.GetAverage() * 1e-6);
        m_EndTime = (double)(m_AverageEndValue.GetAverage() * 1e-6);
        m_ElapsedTime = m_EndTime - m_StartTime;
    }
}

void InAppGpuQueryZone::DrawDetails() {
    if (m_StartFrameId) {
        bool res = false;

        ImGui::TableNextColumn();  // tree

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen;

        if (zonesOrdered.empty())
            flags |= ImGuiTreeNodeFlags_Leaf;

        if (m_Highlighted)
            flags |= ImGuiTreeNodeFlags_Framed;

        if (m_IsRoot) {
            res = ImGui::TreeNodeEx(this, flags, "(%u) %s %u : GPU %.2f ms", depth, name.c_str(), m_StartFrameId - 1U, m_ElapsedTime);
        } else {
            res = ImGui::TreeNodeEx(this, flags, "(%u) %s => GPU %.2f ms", depth, name.c_str(), m_ElapsedTime);
        }

        if (ImGui::IsItemHovered())
            m_Highlighted = true;

        ImGui::TableNextColumn();  // start time
        ImGui::Text("%.5f", m_StartTime);
        ImGui::TableNextColumn();  // end time
        ImGui::Text("%.5f", m_EndTime);
        ImGui::TableNextColumn();  // Elapsed time
        ImGui::Text("%.5f", m_ElapsedTime);

        if (res) {
            m_Expanded = true;

            ImGui::Indent();

            for (const auto zone : zonesOrdered) {
                if (zone != nullptr) {
                    zone->DrawDetails();
                }
            }

            ImGui::Unindent();

            ImGui::TreePop();
        } else {
            m_Expanded = false;
        }
    }
}

bool InAppGpuQueryZone::DrawFlamGraph(InAppGpuGraphTypeEnum vGraphType, IAGPQueryZoneWeak& vOutSelectedQuery, IAGPQueryZoneWeak vParent,
                                      uint32_t vDepth) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) {
        return false;
    }

    bool pressed = false;
    switch (vGraphType) {
        case InAppGpuGraphTypeEnum::IN_APP_GPU_HORIZONTAL:
            pressed = m_DrawHorizontalFlameGraph(m_This.lock(), vOutSelectedQuery, vParent, vDepth);
            break;
        case InAppGpuGraphTypeEnum::IN_APP_GPU_CIRCULAR: pressed = m_DrawCircularFlameGraph(m_This.lock(), vOutSelectedQuery, vParent, vDepth); break;
    }
    return pressed;
}

void InAppGpuQueryZone::updateBreadCrumbTrail() {
    if (parentPtr != nullptr) {
        int32_t _depth = depth;
        IAGPQueryZonePtr _parent_ptr = m_This.lock();
        while (_parent_ptr != rootPtr) {
            _parent_ptr = _parent_ptr->parentPtr;
            if (_parent_ptr && _parent_ptr->depth == (_depth - 1U)) {
                _depth = _parent_ptr->depth;
                if (_depth < m_BreadCrumbTrail.size()) {
                    m_BreadCrumbTrail[_depth] = _parent_ptr;
                } else {
                    DEBUG_BREAK;
                    // maybe you need to define greater value for RECURSIVE_LEVELS_COUNT
                    break;
                }
            }
        }

        // update the imgui title
        imGuiTitle.clear();
        for (GLuint idx = 0U; idx < depth; ++idx) {
            if (idx < m_BreadCrumbTrail.size()) {
                auto ptr = m_BreadCrumbTrail[idx].lock();
                if (ptr != nullptr) {
                    if (idx > 0U) {
                        imGuiTitle += " > ";
                    }
                    imGuiTitle += ptr->name;
                }
            } else {
                DEBUG_BREAK;
                // maybe you need to define greater value for RECURSIVE_LEVELS_COUNT
                break;
            }
        }
        // add the current
        imGuiTitle += " > " + name;

        // add the unicity string
        imGuiTitle += "##InAppGpuQueryZone_ " + std::to_string((intptr_t)this);
    }
}

void InAppGpuQueryZone::DrawBreadCrumbTrail(IAGPQueryZoneWeak& vOutSelectedQuery) {
    ImGui::PushID("DrawBreadCrumbTrail");
    for (GLuint idx = 0U; idx < depth; ++idx) {
        if (idx < m_BreadCrumbTrail.size()) {
            auto ptr = m_BreadCrumbTrail[idx].lock();
            if (ptr != nullptr) {
                if (idx > 0U) {
                    ImGui::SameLine();
                    ImGui::Text("%s", ">");
                    ImGui::SameLine();
                }
                if (IAGP_IMGUI_BUTTON(ptr->imGuiLabel.c_str())) {
                    vOutSelectedQuery = m_BreadCrumbTrail[idx];
                }
            }
        } else {
            DEBUG_BREAK;
            // maybe you need to define greater value for RECURSIVE_LEVELS_COUNT
            break;
        }
    }
    if (depth > 0) {
        ImGui::SameLine();
        ImGui::Text("> %s", name.c_str());
    }
    ImGui::PopID();
}

bool InAppGpuQueryZone::m_ComputeRatios(IAGPQueryZonePtr vRoot, IAGPQueryZoneWeak vParent, uint32_t vDepth, float& vOutStartRatio,
                                        float& vOutSizeRatio) {
    if (depth > InAppGpuQueryZone::sMaxDepthToOpen) {
        return false;
    }
    if (vRoot == nullptr) {
        vRoot = m_This.lock();
    }
    if (vParent.expired()) {
        vParent = m_This;
    }
    if (vRoot != nullptr && vRoot->m_ElapsedTime > 0.0) {  // avoid div by zero
        if (vDepth == 0) {
            vOutStartRatio = 0.0f;
            vOutSizeRatio = 1.0f;
            hsv = ImVec4((float)(0.5 - m_ElapsedTime * 0.5 / vRoot->m_ElapsedTime), 0.5f, 1.0f, 1.0f);
        } else {
            auto parent_ptr = vParent.lock();
            if (parent_ptr) {
                if (parent_ptr->m_ElapsedTime > 0.0) {  // avoid div by zero

                    ////////////////////////////////////////////////////////
                    // for correct rounding isssue with average values
                    if (parent_ptr->m_StartTime > m_StartTime) {
                        m_StartTime = parent_ptr->m_StartTime;
                    }
                    if (parent_ptr->m_EndTime < m_EndTime) {
                        m_EndTime = parent_ptr->m_EndTime;
                    }
                    if (m_EndTime < m_StartTime) {
                        m_EndTime = m_StartTime;
                    }
                    m_ElapsedTime = m_EndTime - m_StartTime;
                    if (m_ElapsedTime < 0.0) {
                        DEBUG_BREAK;
                    }
                    if (m_ElapsedTime > parent_ptr->m_ElapsedTime) {
                        m_ElapsedTime = parent_ptr->m_ElapsedTime;
                    }
                    ////////////////////////////////////////////////////////

                    vOutStartRatio = (float)((m_StartTime - vRoot->m_StartTime) / vRoot->m_ElapsedTime);
                    vOutSizeRatio = (float)(m_ElapsedTime / vRoot->m_ElapsedTime);
                    hsv = ImVec4((float)(0.5 - 0.5 * vOutSizeRatio), 0.5f, 1.0f, 1.0f);
                }
            }
        }
        return true;
    }
    return false;
}

bool InAppGpuQueryZone::m_DrawHorizontalFlameGraph(IAGPQueryZonePtr vRoot, IAGPQueryZoneWeak& vOutSelectedQuery, IAGPQueryZoneWeak vParent,
                                                   uint32_t vDepth) {
    bool pressed = false;
    const ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const float aw = ImGui::GetContentRegionAvail().x - style.FramePadding.x;
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    float barStartRatio = 0.0f;
    float barSizeRatio = 0.0f;
    if (m_ComputeRatios(vRoot, vParent, vDepth, barStartRatio, barSizeRatio)) {
        if (barSizeRatio > 0.0f) {
            if ((zonesOrdered.empty() && InAppGpuQueryZone::sShowLeafMode) || !InAppGpuQueryZone::sShowLeafMode) {
                ImGui::PushID(this);
                m_BarLabel = toStr("%s (%.1f ms | %.1f f/s)", name.c_str(), m_ElapsedTime, 1000.0f / m_ElapsedTime);
                const char* label = m_BarLabel.c_str();
                const ImGuiID id = window->GetID(label);
                ImGui::PopID();
                float bar_start = aw * barStartRatio;
                float bar_size = aw * barSizeRatio;
                const ImVec2 label_size = ImGui::CalcTextSize(label, nullptr, true);
                const float height = label_size.y + style.FramePadding.y * 2.0f;
                const ImVec2 bPos = ImVec2(bar_start + style.FramePadding.x, vDepth * height + style.FramePadding.y);
                const ImVec2 bSize = ImVec2(bar_size - style.FramePadding.x, 0.0f);
                const ImVec2 pos = window->DC.CursorPos + bPos;
                const ImVec2 size = ImVec2(bar_size, height);
                const ImRect bb(pos, pos + size);
                bool hovered, held;
                pressed =
                    ImGui::ButtonBehavior(bb, id, &hovered, &held,  //
                                          ImGuiButtonFlags_PressedOnClick | ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
                if (pressed) {
                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                        vOutSelectedQuery = m_This;  // open in the main window
                    } else if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                        sTabbedQueryZones.push_back(m_This);  // open new window
                    }
                }
                m_Highlighted = false;
                if (hovered) {
                    ImGui::SetTooltip("Section : [%s : %s]\nElapsed time : %.05f ms\nElapsed FPS : %.05f f/s",  //
                                      m_SectionName.c_str(), name.c_str(), m_ElapsedTime, 1000.0f / m_ElapsedTime);
                    m_Highlighted = true;  // to highlight label graph by this button
                } else if (m_Highlighted) {
                    hovered = true;  // highlight this button by the label graph
                }
                ImGui::ColorConvertHSVtoRGB(hsv.x, hsv.y, hsv.z, cv4.x, cv4.y, cv4.z);
                cv4.w = 1.0f;
                ImGui::RenderNavHighlight(bb, id);
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
                const auto colorU32 = ImGui::ColorConvertFloat4ToU32(cv4);
                ImGui::RenderFrame(bb.Min, bb.Max, colorU32, true, 2.0f);
                if (hovered) {
                    const auto selectU32 = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f - cv4.x, 1.0f - cv4.y, 1.0f - cv4.z, 1.0f));
                    window->DrawList->AddRect(bb.Min, bb.Max, selectU32, true, 0, 2.0f);
                }
                ImGui::PopStyleVar();
                const bool pushed = PushStyleColorWithContrast(colorU32, ImGuiCol_Text, ImVec4(0, 0, 0, 1), InAppGpuQueryZone::sContrastRatio);
                ImGui::RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, label, nullptr, &label_size,
                                         ImVec2(0.5f, 0.5f) /*style.ButtonTextAlign*/, &bb);
                if (pushed) {
                    ImGui::PopStyleColor();
                }
                ++vDepth;
            }

            // we dont show child if this one have elapsed time to 0.0
            for (const auto zone : zonesOrdered) {
                if (zone != nullptr) {
                    pressed |= zone->m_DrawHorizontalFlameGraph(vRoot, vOutSelectedQuery, m_This, vDepth);
                }
            }
        } else {
#ifdef DEBUG_MODE_LOGGING
            DEBUG_MODE_LOGGING("Bar Ms not displayed", name.c_str());
#endif
        }
    }

    if (depth == 0 && ((zonesOrdered.empty() && InAppGpuQueryZone::sShowLeafMode) || !InAppGpuQueryZone::sShowLeafMode)) {
        const ImVec2 pos = window->DC.CursorPos;
        const ImVec2 size = ImVec2(aw, ImGui::GetFrameHeight() * (InAppGpuScopedZone::sMaxDepth + 1U));
        ImGui::ItemSize(size);
        const ImRect bb(pos, pos + size);
        const ImGuiID id = window->GetID((name + "##canvas").c_str());
        if (!ImGui::ItemAdd(bb, id)) {
            return pressed;
        }
    }

    return pressed;
}

bool InAppGpuQueryZone::m_DrawCircularFlameGraph(IAGPQueryZonePtr vRoot, IAGPQueryZoneWeak& vOutSelectedQuery, IAGPQueryZoneWeak vParent,
                                                 uint32_t vDepth) {
    bool pressed = false;

    if (vDepth == 0U) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Settings")) {
                ImGui::SliderFloat("count points", &sCircularSettings.count_point, 1.0f, 240.0f);
                ImGui::SliderFloat("base_radius", &sCircularSettings.base_radius, 0.0f, 240.0f);
                ImGui::SliderFloat("space", &sCircularSettings.space, 0.0f, 240.0f);
                ImGui::SliderFloat("thick", &sCircularSettings.thick, 0.0f, 240.0f);
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
    }

    ImGuiWindow* window = ImGui::GetCurrentWindow();
    float barStartRatio = 0.0f;
    float barSizeRatio = 0.0f;
    if (m_ComputeRatios(vRoot, vParent, vDepth, barStartRatio, barSizeRatio)) {
        if (barSizeRatio > 0.0f) {
            if ((zonesOrdered.empty() && InAppGpuQueryZone::sShowLeafMode) || !InAppGpuQueryZone::sShowLeafMode) {
                ImVec2 center = window->DC.CursorPos + ImGui::GetContentRegionAvail() * 0.5f;
                ImGui::ColorConvertHSVtoRGB(hsv.x, hsv.y, hsv.z, cv4.x, cv4.y, cv4.z);
                cv4.w = 1.0f;

                float min_radius = sCircularSettings.base_radius + sCircularSettings.space * vDepth + sCircularSettings.thick * vDepth;
                float max_radius = sCircularSettings.base_radius + sCircularSettings.space * vDepth + sCircularSettings.thick * (vDepth + 1U);

                auto draw_list_ptr = window->DrawList;
                auto colU32 = ImGui::GetColorU32(cv4);

                float full_length = _1PI_;
                float full_offset = _1PI_;

                float base_st = full_length / sCircularSettings.count_point;

                float bar_start = full_length * barStartRatio;
                float bar_size = full_length * (barStartRatio + barSizeRatio);
                float st = bar_size / ImMax(floor(bar_size / base_st), 3.0f);  // 2 points mini par barre

                ImVector<ImVec2> path;
                float co = 0.0f, si = 0.0f, ac = 0.0f, oc = 0.0f;
                for (ac = bar_start; ac < bar_size; ac += st) {
                    ac = ImMin(ac, bar_size);
                    oc = ac + full_offset;
                    co = std::cos(oc) * sCircularSettings.scaleX;
                    si = std::sin(oc) * sCircularSettings.scaleY;
                    m_P0.x = co * min_radius + center.x;
                    m_P0.y = si * min_radius + center.y;
                    m_P1.x = co * max_radius + center.x;
                    m_P1.y = si * max_radius + center.y;
                    if (ac > bar_start) {
                        // draw_list_ptr->AddQuadFilled(m_P0, m_P1, m_LP1, m_LP0, colU32);
                        draw_list_ptr->AddQuad(m_P0, m_P1, m_LP1, m_LP0, colU32, 2.0f);  // m_BlackU32

                        // draw_list_ptr->PathLineTo(m_P0);
                        // draw_list_ptr->PathLineTo(m_P1);
                        // draw_list_ptr->PathLineTo(m_LP1);
                        // draw_list_ptr->PathLineTo(m_LP0);
                    }
                    m_LP0 = m_P0;
                    m_LP1 = m_P1;
                }
                // draw_list_ptr->PathStroke(colU32, ImDrawFlags_Closed, 2.0f);

#ifdef _DEBUG
                // draw_list_ptr->AddLine(center, center - ImVec2(0, 150.0f), colU32, 2.0f);
#endif

                ++vDepth;
            }

            // we dont show child if this one have elapsed time to 0.0
            // childs
            for (const auto zone : zonesOrdered) {
                if (zone != nullptr) {
                    pressed |= zone->m_DrawCircularFlameGraph(vRoot, vOutSelectedQuery, m_This, vDepth);
                }
            }
        }
    }

    return pressed;
}

////////////////////////////////////////////////////////////
/////////////////////// GL CONTEXT /////////////////////////
////////////////////////////////////////////////////////////

IAGPContextPtr InAppGpuGLContext::create(GPU_CONTEXT vContext) {
    auto res = std::make_shared<InAppGpuGLContext>(vContext);
    res->m_This = res;
    return res;
}

InAppGpuGLContext::InAppGpuGLContext(GPU_CONTEXT vContext) : m_Context(vContext) {
}

void InAppGpuGLContext::Clear() {
    m_RootZone.reset();
    m_PendingUpdate.clear();
    m_QueryIDToZone.clear();
    m_DepthToLastZone.clear();
}

void InAppGpuGLContext::Init() {
}

void InAppGpuGLContext::Unit() {
    Clear();
}

void InAppGpuGLContext::Collect() {
#ifdef DEBUG_MODE_LOGGING
    DEBUG_MODE_LOGGING("------ Collect Trhead (%i) -----", (intptr_t)m_Context);
#endif

    auto it = m_PendingUpdate.begin();
    while (!m_PendingUpdate.empty() && it != m_PendingUpdate.end()) {
        GLuint id = *it;
        GLuint value = 0;
        glGetQueryObjectuiv(id, GL_QUERY_RESULT_AVAILABLE, &value);
        const auto it_to_erase_eventually = it;
        ++it;
        if (value == GL_TRUE /* || id == m_RootZone->ids[0] || id == m_RootZone->ids[1]*/) {
            GLuint64 value64 = 0;
            glGetQueryObjectui64v(id, GL_QUERY_RESULT, &value64);
            if (m_QueryIDToZone.find(id) != m_QueryIDToZone.end()) {
                auto ptr = m_QueryIDToZone[id];
                if (ptr != nullptr) {
                    if (id == ptr->ids[0])
                        ptr->SetStartTimeStamp(value64);
                    else if (id == ptr->ids[1])
                        ptr->SetEndTimeStamp(value64);
                    else
                        DEBUG_BREAK;
                }
            }
            m_PendingUpdate.erase(it_to_erase_eventually);
        } else {
            auto ptr = m_QueryIDToZone[id];
            if (ptr != nullptr) {
                LOG_ERROR_MESSAGE("%*s id not retrieved : %u", ptr->depth, "", id);
            }
        }
    }

#ifdef DEBUG_MODE_LOGGING
    DEBUG_MODE_LOGGING("------ End Frame -----");
#endif
}

void InAppGpuGLContext::DrawFlamGraph(const InAppGpuGraphTypeEnum& vGraphType) {
    if (m_RootZone != nullptr) {
        if (!m_SelectedQuery.expired()) {
            auto ptr = m_SelectedQuery.lock();
            if (ptr) {
                ptr->DrawBreadCrumbTrail(m_SelectedQuery);
                ptr->DrawFlamGraph(vGraphType, m_SelectedQuery);
            }
        } else {
            m_RootZone->DrawFlamGraph(vGraphType, m_SelectedQuery);
        }
    }
}

void InAppGpuGLContext::DrawDetails() {
    if (m_RootZone != nullptr) {
        m_RootZone->DrawDetails();
    }
}

IAGPQueryZonePtr InAppGpuGLContext::GetQueryZoneForName(const std::string& vName, const std::string& vSection, const bool& vIsRoot) {
    IAGPQueryZonePtr res = nullptr;

    /////////////////////////////////////////////
    //////////////// CREATION ///////////////////
    /////////////////////////////////////////////

    // there is many link issues with 'max' in cross compilation so we dont using it
    if (InAppGpuScopedZone::sCurrentDepth > InAppGpuScopedZone::sMaxDepth) {
        InAppGpuScopedZone::sMaxDepth = InAppGpuScopedZone::sCurrentDepth;
    }

    if (InAppGpuScopedZone::sCurrentDepth == 0) {  // root zone
#ifdef DEBUG_MODE_LOGGING
        DEBUG_MODE_LOGGING("------ Start Frame -----");
#endif
        m_DepthToLastZone.clear();
        if (m_RootZone == nullptr) {
            res = InAppGpuQueryZone::create(m_Context, vName, vSection, vIsRoot);
            if (res != nullptr) {
                res->depth = InAppGpuScopedZone::sCurrentDepth;
                res->updateBreadCrumbTrail();
                m_QueryIDToZone[res->ids[0]] = res;
                m_QueryIDToZone[res->ids[1]] = res;
                m_RootZone = res;
#ifdef DEBUG_MODE_LOGGING
                // DEBUG_MODE_LOGGING("Profile : add zone %s at puDepth %u", vName.c_str(), InAppGpuScopedZone::sCurrentDepth);
#endif
            }
        } else {
            res = m_RootZone;
        }
    } else {  // else child zone
        auto root = m_GetQueryZoneFromDepth(InAppGpuScopedZone::sCurrentDepth - 1U);
        if (root != nullptr) {
            if (root->zonesDico.find(vName) == root->zonesDico.end()) {  // not found
                res = InAppGpuQueryZone::create(m_Context, vName, vSection, vIsRoot);
                if (res != nullptr) {
                    res->parentPtr = root;
                    res->rootPtr = m_RootZone;
                    res->depth = InAppGpuScopedZone::sCurrentDepth;
                    res->updateBreadCrumbTrail();
                    m_QueryIDToZone[res->ids[0]] = res;
                    m_QueryIDToZone[res->ids[1]] = res;
                    root->zonesDico[vName] = res;
                    root->zonesOrdered.push_back(res);
#ifdef DEBUG_MODE_LOGGING
                    // DEBUG_MODE_LOGGING("Profile : add zone %s at puDepth %u", vName.c_str(), InAppGpuScopedZone::sCurrentDepth);
#endif
                } else {
                    DEBUG_BREAK;
                }
            } else {
                res = root->zonesDico[vName];
            }
        } else {
            return res;  // happen when profiling is activated inside a profiling zone
        }
    }

    /////////////////////////////////////////////
    //////////////// UTILISATION ////////////////
    /////////////////////////////////////////////

    if (res != nullptr) {
        m_SetQueryZoneForDepth(res, InAppGpuScopedZone::sCurrentDepth);

        if (res->name != vName) {
            // at depth 0 there is only one frame
            LOG_DEBUG_ERROR_MESSAGE("was registerd at depth %u %s. but we got %s\nwe clear the profiler",  //
                                    InAppGpuScopedZone::sCurrentDepth, res->name.c_str(), vName.c_str());
            // maybe the scoped frame is taken outside of the main frame
            Clear();
        }

        m_PendingUpdate.emplace(res->ids[0]);
        m_PendingUpdate.emplace(res->ids[1]);
    }

    return res;
}

void InAppGpuGLContext::m_SetQueryZoneForDepth(IAGPQueryZonePtr vInAppGpuQueryZone, GLuint vDepth) {
    m_DepthToLastZone[vDepth] = vInAppGpuQueryZone;
}

IAGPQueryZonePtr InAppGpuGLContext::m_GetQueryZoneFromDepth(GLuint vDepth) {
    IAGPQueryZonePtr res = nullptr;

    if (m_DepthToLastZone.find(vDepth) != m_DepthToLastZone.end()) {  // found
        res = m_DepthToLastZone[vDepth];
    }

    return res;
}

////////////////////////////////////////////////////////////
/////////////////////// GL PROFILER ////////////////////////
////////////////////////////////////////////////////////////

bool InAppGpuProfiler::sIsActive = false;
bool InAppGpuProfiler::sIsPaused = false;

InAppGpuProfiler::InAppGpuProfiler() = default;
InAppGpuProfiler::InAppGpuProfiler(const InAppGpuProfiler&) = default;

InAppGpuProfiler& InAppGpuProfiler::operator=(const InAppGpuProfiler&) {
    return *this;
};

InAppGpuProfiler::~InAppGpuProfiler() {
    Clear();
};

void InAppGpuProfiler::Clear() {
    m_Contexts.clear();
}

void InAppGpuProfiler::Collect() {
    if (!sIsActive || sIsPaused) {
        return;
    }

    glFinish();

    for (const auto& con : m_Contexts) {
        if (con.second != nullptr) {
            con.second->Collect();
        }
    }
}

void InAppGpuProfiler::DrawFlamGraph(const char* vLabel, bool* pOpen, ImGuiWindowFlags vFlags) {
    if (m_ImGuiBeginFunctor != nullptr && m_ImGuiBeginFunctor(vLabel, pOpen, vFlags | ImGuiWindowFlags_MenuBar)) {
        DrawFlamGraphNoWin();
    }
    if (m_ImGuiEndFunctor != nullptr) {
        m_ImGuiEndFunctor();
    }

    DrawFlamGraphChilds(vFlags);
}

void InAppGpuProfiler::DrawFlamGraphNoWin() {
    if (sIsActive) {
        m_DrawMenuBar();
        for (const auto& con : m_Contexts) {
            if (con.second != nullptr) {
                con.second->DrawFlamGraph(m_GraphType);
            }
        }
    }
}

void InAppGpuProfiler::DrawFlamGraphChilds(ImGuiWindowFlags vFlags) {
    m_SelectedQuery.reset();
    m_QueryZoneToClose = -1;
    for (size_t idx = 0U; idx < iagp::InAppGpuQueryZone::sTabbedQueryZones.size(); ++idx) {
        auto ptr = iagp::InAppGpuQueryZone::sTabbedQueryZones[idx].lock();
        if (ptr != nullptr) {
            bool opened = true;
            ImGui::SetNextWindowSizeConstraints(SUB_AIGP_WINDOW_MIN_SIZE, ImGui::GetIO().DisplaySize);
            if (m_ImGuiBeginFunctor != nullptr && m_ImGuiBeginFunctor(ptr->imGuiTitle.c_str(), &opened, vFlags)) {
                if (sIsActive) {
                    ptr->DrawFlamGraph(iagp::InAppGpuProfiler::Instance()->GetGraphTypeRef(), m_SelectedQuery);
                }
            }
            if (m_ImGuiEndFunctor != nullptr) {
                m_ImGuiEndFunctor();
            }
            if (!opened) {
                m_QueryZoneToClose = (int32_t)idx;
            }
        }
    }
    if (m_QueryZoneToClose > -1) {
        iagp::InAppGpuQueryZone::sTabbedQueryZones.erase(  //
            iagp::InAppGpuQueryZone::sTabbedQueryZones.begin() + m_QueryZoneToClose);
    }
}

void InAppGpuProfiler::SetImGuiBeginFunctor(const ImGuiBeginFunctor& vImGuiBeginFunctor) {
    m_ImGuiBeginFunctor = vImGuiBeginFunctor;
}

void InAppGpuProfiler::SetImGuiEndFunctor(const ImGuiEndFunctor& vImGuiEndFunctor) {
    m_ImGuiEndFunctor = vImGuiEndFunctor;
}

void InAppGpuProfiler::m_DrawMenuBar() {
    if (ImGui::BeginMenuBar()) {
        if (InAppGpuScopedZone::sMaxDepth) {
            InAppGpuQueryZone::sMaxDepthToOpen = InAppGpuScopedZone::sMaxDepth;
        }

        IMGUI_PLAY_PAUSE_BUTTON(sIsPaused);

        //ImGui::Checkbox("Logging", &InAppGpuQueryZone::sActivateLogger);

        /*if (ImGui::BeginMenu("Graph Types")) {
            if (ImGui::MenuItem("Horizontal", nullptr, m_GraphType == iagp::InAppGpuGraphTypeEnum::IN_APP_GPU_HORIZONTAL)) {
                m_GraphType = iagp::InAppGpuGraphTypeEnum::IN_APP_GPU_HORIZONTAL;
            }
            if (ImGui::MenuItem("Circular", nullptr, m_GraphType == iagp::InAppGpuGraphTypeEnum::IN_APP_GPU_CIRCULAR)) {
                m_GraphType = iagp::InAppGpuGraphTypeEnum::IN_APP_GPU_CIRCULAR;
            }
            ImGui::EndMenu();
        }*/

        ImGui::EndMenuBar();
    }
}

void InAppGpuProfiler::DrawDetails() {
    if (!sIsActive) {
        return;
    }

    static ImGuiTableFlags flags =        //
        ImGuiTableFlags_SizingFixedFit |  //
        ImGuiTableFlags_RowBg |           //
        ImGuiTableFlags_Hideable |        //
        ImGuiTableFlags_ScrollY |         //
        ImGuiTableFlags_NoHostExtendY;
    const auto& size = ImGui::GetContentRegionAvail();
    auto listViewID = ImGui::GetID("##InAppGpuProfiler_DrawDetails");
    if (ImGui::BeginTableEx("##InAppGpuProfiler_DrawDetails", listViewID, 4, flags, size, 0.0f)) {
        ImGui::TableSetupColumn("Tree");
        ImGui::TableSetupColumn("Start time");
        ImGui::TableSetupColumn("End time");
        ImGui::TableSetupColumn("Elapsed time");
        ImGui::TableHeadersRow();
        for (const auto& con : m_Contexts) {
            if (con.second != nullptr) {
                con.second->DrawDetails();
            }
        }
        ImGui::EndTable();
    }
}

IAGPContextPtr InAppGpuProfiler::GetContextPtr(GPU_CONTEXT vThreadPtr) {
    if (!sIsActive) {
        return nullptr;
    }

    if (vThreadPtr != nullptr) {
        if (m_Contexts.find((intptr_t)vThreadPtr) == m_Contexts.end()) {
            m_Contexts[(intptr_t)vThreadPtr] = InAppGpuGLContext::create(vThreadPtr);
        }

        return m_Contexts[(intptr_t)vThreadPtr];
    }

    LOG_ERROR_MESSAGE("GPU_CONTEXT vThreadPtr is NULL");

    return nullptr;
}

////////////////////////////////////////////////////////////
/////////////////////// SCOPED ZONE ////////////////////////
////////////////////////////////////////////////////////////

// STATIC
uint32_t InAppGpuScopedZone::sCurrentDepth = 0U;
uint32_t InAppGpuScopedZone::sMaxDepth = 0U;

// SCOPED ZONE
InAppGpuScopedZone::InAppGpuScopedZone(const bool& vIsRoot, const std::string& vSection, const char* fmt, ...) {
    if (InAppGpuProfiler::sIsActive) {
        va_list args;
        va_start(args, fmt);
        static char TempBuffer[256];
        const int w = vsnprintf(TempBuffer, 256, fmt, args);
        va_end(args);
        if (w) {
            auto context_ptr = InAppGpuProfiler::Instance()->GetContextPtr(GET_CURRENT_CONTEXT());
            if (context_ptr != nullptr) {
                const auto& label = std::string(TempBuffer, (size_t)w);
                queryPtr = context_ptr->GetQueryZoneForName(label, vSection, vIsRoot);
                if (queryPtr != nullptr) {
                    glQueryCounter(queryPtr->ids[0], GL_TIMESTAMP);
#ifdef DEBUG_MODE_LOGGING
                    DEBUG_MODE_LOGGING("%*s begin : [%u:%u] (depth:%u) (%s)",  //
                                       queryPtr->depth, "", queryPtr->ids[0], queryPtr->ids[1], queryPtr->depth, label.c_str());
#endif
                    sCurrentDepth++;
                }
            }
        }
    }
}

InAppGpuScopedZone::~InAppGpuScopedZone() {
    if (queryPtr != nullptr) {
#ifdef DEBUG_MODE_LOGGING
        if (queryPtr->depth > 0) {
            DEBUG_MODE_LOGGING("%*s end : [%u:%u] (depth:%u)",  //
                               (queryPtr->depth - 1U), "", queryPtr->ids[0], queryPtr->ids[1], queryPtr->depth);
        } else {
            DEBUG_MODE_LOGGING("end : [%u:%u] (depth:%u)",  //
                               queryPtr->ids[0], queryPtr->ids[1], 0);
        }
#endif
        glQueryCounter(queryPtr->ids[1], GL_TIMESTAMP);
        sCurrentDepth--;
    }
}

}  // namespace iagp
