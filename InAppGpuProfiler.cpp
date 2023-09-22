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

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif  // IMGUI_DEFINE_MATH_OPERATORS

#include <imgui.h>
#include <imgui_internal.h>

#ifdef _MSC_VER
#define DEBUG_BREAK          \
    if (IsDebuggerPresent()) \
    __debugbreak()
#else
#define DEBUG_BREAK
#endif

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
        DEBUG_BREAK;
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
    : m_Context(vContext), puName(vName), m_IsRoot(vIsRoot), m_SectionName(vSectionName) {
    m_StartFrameId = 0;
    m_EndFrameId = 0;
    m_StartTimeStamp = 0;
    m_EndTimeStamp = 0;
    m_ElapsedTime = 0.0;
    puDepth = InAppGpuScopedZone::sCurrentDepth;

    SET_CURRENT_CONTEXT(m_Context);
    CheckGLErrors;
    glGenQueries(2, puIds);
    CheckGLErrors;
}

InAppGpuQueryZone::~InAppGpuQueryZone() {
    SET_CURRENT_CONTEXT(m_Context);
    CheckGLErrors;
    glDeleteQueries(2, puIds);
    CheckGLErrors;

    puName.clear();
    m_StartFrameId = 0;
    m_EndFrameId = 0;
    m_StartTimeStamp = 0;
    m_EndTimeStamp = 0;
    m_ElapsedTime = 0.0;
    puZonesOrdered.clear();
    puZonesDico.clear();
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
    DEBUG_MODE_LOGGING("%*s end id retrieved : %u", puDepth, "", puIds[1]);
#endif
    // start computation of elapsed time
    // no needed after
    // will be used for Graph and labels
    // so DrawMetricGraph must be the first
    ComputeElapsedTime();

    if (InAppGpuQueryZone::sActivateLogger && puZonesOrdered.empty())  // only the leafs
    {
        /*double v = (double)vValue / 1e9;
        LogVarLightInfo("<m_ofiler section=\"%s\" epoch_time=\"%f\" name=\"%s\" render_time_ms=\"%f\">",
            m_SectionName.c_str(), v, puName.c_str(), m_ElapsedTime);*/
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

        if (puZonesOrdered.empty())
            flags |= ImGuiTreeNodeFlags_Leaf;

        if (m_Highlighted)
            flags |= ImGuiTreeNodeFlags_Framed;

        if (m_IsRoot) {
            res = ImGui::TreeNodeEx(this, flags, "(%u) %s %u : GPU %.2f ms", puDepth, puName.c_str(), m_StartFrameId - 1U, m_ElapsedTime);
        } else {
            res = ImGui::TreeNodeEx(this, flags, "(%u) %s => GPU %.2f ms", puDepth, puName.c_str(), m_ElapsedTime);
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

            for (const auto zone : puZonesOrdered) {
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

bool InAppGpuQueryZone::DrawFlamGraph(InAppGpuGraphTypeEnum vGraphType, IAGPQueryZonePtr vParent, uint32_t vDepth) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) {
        return false;
    }
    
    bool pressed = false;
    switch (vGraphType) {
        case InAppGpuGraphTypeEnum::IN_APP_GPU_HORIZONTAL: pressed = m_DrawHorizontalFlameGraph(vParent, vDepth); break;
        case InAppGpuGraphTypeEnum::IN_APP_GPU_CIRCULAR: pressed = m_DrawCircularFlameGraph(vParent, vDepth); break;
    }
    return pressed;
}

bool InAppGpuQueryZone::m_ComputeRatios(IAGPQueryZonePtr vParent, uint32_t vDepth) {
    if (puDepth > InAppGpuQueryZone::sMaxDepthToOpen) {
        return false;
    }
    if (vParent == nullptr) {
        vParent = puThis;
    }
    if (puTopQuery == nullptr) {
        puTopQuery = puThis;
    }
    if (puTopQuery != nullptr && m_ElapsedTime > 0.0) {
        if (vDepth == 0) {
            m_BarSizeRatio = 1.0f;
            m_BarStartRatio = 0.0f;
            hsv = ImVec4((float)(0.5 - m_ElapsedTime * 0.5 / puTopQuery->m_ElapsedTime), 0.5f, 1.0f, 1.0f);
        } else if (vParent->m_ElapsedTime > 0.0) {
            puTopQuery = vParent->puTopQuery;

            ////////////////////////////////////////////////////////
            // for correct rounding isssue with average values
            if (vParent->m_StartTime > m_StartTime) {
                m_StartTime = vParent->m_StartTime;
            }
            if (vParent->m_EndTime < m_EndTime) {
                m_EndTime = vParent->m_EndTime;
            }
            if (m_EndTime < m_StartTime) {
                m_EndTime = m_StartTime;
            }
            m_ElapsedTime = m_EndTime - m_StartTime;
            if (m_ElapsedTime < 0.0) {
                DEBUG_BREAK;
            }
            if (m_ElapsedTime > vParent->m_ElapsedTime) {
                m_ElapsedTime = vParent->m_ElapsedTime;
            }
            ////////////////////////////////////////////////////////

            m_BarStartRatio = (m_StartTime - puTopQuery->m_StartTime) / puTopQuery->m_ElapsedTime;
            m_BarSizeRatio = m_ElapsedTime / puTopQuery->m_ElapsedTime;
            hsv = ImVec4((float)(0.5 - m_ElapsedTime * 0.5 / puTopQuery->m_ElapsedTime), 0.5f, 1.0f, 1.0f);
        }
        return true;
    }
    return false;
}

bool InAppGpuQueryZone::m_DrawHorizontalFlameGraph(IAGPQueryZonePtr vParent, uint32_t vDepth) {
    bool pressed = false;
    const ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const float aw = ImGui::GetContentRegionAvail().x - style.FramePadding.x;
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (m_ComputeRatios(vParent, vDepth)){
        if (m_BarSizeRatio > 0.0f) {
            if ((puZonesOrdered.empty() && InAppGpuQueryZone::sShowLeafMode) || !InAppGpuQueryZone::sShowLeafMode) {
                ImGui::PushID(this);
                m_BarLabel = toStr("%s (%.1f ms | %.1f f/s)", puName.c_str(), m_ElapsedTime, 1000.0f / m_ElapsedTime);
                const char* label = m_BarLabel.c_str();
                const ImGuiID id = window->GetID(label);
                ImGui::PopID();
                float bar_start = aw * m_BarStartRatio;
                float bar_size = aw * m_BarSizeRatio;
                const ImVec2 label_size = ImGui::CalcTextSize(label, nullptr, true);
                const float height = label_size.y + style.FramePadding.y * 2.0f;
                const ImVec2 bPos = ImVec2(bar_start + style.FramePadding.x, vDepth * height + style.FramePadding.y);
                const ImVec2 bSize = ImVec2(bar_size - style.FramePadding.x, 0.0f);
                const ImVec2 pos = window->DC.CursorPos + bPos;
                const ImVec2 size = ImVec2(bar_size, height);
                const ImRect bb(pos, pos + size);
                bool hovered, held;
                pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, ImGuiButtonFlags_PressedOnClick);
                if (pressed) {
                    sTabbedQueryZones.push_back(m_This);
                }
                m_Highlighted = false;
                if (hovered) {
                    ImGui::SetTooltip("section %s : %s\nElapsed time : %.05f ms\nElapsed FPS : %.05f f/s",  //
                                      m_SectionName.c_str(), puName.c_str(), m_ElapsedTime, 1000.0f / m_ElapsedTime);
                    m_Highlighted = true;  // to highlight label graph by this button
                } else if (m_Highlighted) {
                    hovered = true;  // highlight this button by the label graph
                }
                ImGui::ColorConvertHSVtoRGB(hsv.x, hsv.y, hsv.z, cv4.x, cv4.y, cv4.z);
                cv4.w = 1.0f;
                ImGui::RenderNavHighlight(bb, id);
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
                ImGui::RenderFrame(bb.Min, bb.Max, ImGui::ColorConvertFloat4ToU32(cv4), true, 2.0f);
                ImGui::PopStyleVar();
                const bool pushed = PushStyleColorWithContrast(  //
                    ImGui::ColorConvertFloat4ToU32(cv4), ImGuiCol_Text, ImVec4(0, 0, 0, 1), InAppGpuQueryZone::sContrastRatio);
                ImGui::RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, label, nullptr, &label_size,
                                         ImVec2(0.5f, 0.5f) /*style.ButtonTextAlign*/, &bb);
                if (pushed) {
                    ImGui::PopStyleColor();
                }
                ++vDepth;
            }

            // we dont show child if this one have elapsed time to 0.0
            for (const auto zone : puZonesOrdered) {
                if (zone != nullptr) {
                    pressed |= zone->m_DrawHorizontalFlameGraph(puThis, vDepth);
                }
            }
        }
    }

    if (puDepth == 0 && ((puZonesOrdered.empty() && InAppGpuQueryZone::sShowLeafMode) || !InAppGpuQueryZone::sShowLeafMode)) {
        const ImVec2 pos = window->DC.CursorPos;
        const ImVec2 size = ImVec2(aw, ImGui::GetFrameHeight() * (InAppGpuScopedZone::sMaxDepth + 1U));
        ImGui::ItemSize(size);
        const ImRect bb(pos, pos + size);
        const ImGuiID id = window->GetID((puName + "##canvas").c_str());
        if (!ImGui::ItemAdd(bb, id)) {
            return pressed;
        }
    }

    return pressed;
}

bool InAppGpuQueryZone::m_DrawCircularFlameGraph(IAGPQueryZonePtr vParent, uint32_t vDepth) {
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
    if (m_ComputeRatios(vParent, vDepth)) {
        if (m_BarSizeRatio > 0.0f) {
            if ((puZonesOrdered.empty() && InAppGpuQueryZone::sShowLeafMode) || !InAppGpuQueryZone::sShowLeafMode) {
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

                float bar_start = full_length * m_BarStartRatio;
                float bar_size = full_length * (m_BarStartRatio + m_BarSizeRatio);
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
                        //draw_list_ptr->AddQuadFilled(m_P0, m_P1, m_LP1, m_LP0, colU32);
                        draw_list_ptr->AddQuad(m_P0, m_P1, m_LP1, m_LP0, colU32, 2.0f); // m_BlackU32

                        //draw_list_ptr->PathLineTo(m_P0);
                        //draw_list_ptr->PathLineTo(m_P1);
                        //draw_list_ptr->PathLineTo(m_LP1);
                        //draw_list_ptr->PathLineTo(m_LP0);
                    }
                    m_LP0 = m_P0;
                    m_LP1 = m_P1;
                }
                //draw_list_ptr->PathStroke(colU32, ImDrawFlags_Closed, 2.0f);

#ifdef _DEBUG
                // draw_list_ptr->AddLine(center, center - ImVec2(0, 150.0f), colU32, 2.0f);
#endif

                ++vDepth;
            }

            // we dont show child if this one have elapsed time to 0.0
            // childs
            for (const auto zone : puZonesOrdered) {
                if (zone != nullptr) {
                    pressed |= zone->m_DrawCircularFlameGraph(puThis, vDepth);
                }
            }
        }
    }

    return pressed;
}

////////////////////////////////////////////////////////////
/////////////////////// GL CONTEXT /////////////////////////
////////////////////////////////////////////////////////////

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
    DEBUG_MODE_LOGGING("------ Collect Trhead (%i) -----", (intptr_t)m_ThreadPtr);
#endif

    auto it = m_PendingUpdate.begin();
    while (!m_PendingUpdate.empty() && it != m_PendingUpdate.end()) {
        GLuint id = *it;
        GLuint value = 0;
        glGetQueryObjectuiv(id, GL_QUERY_RESULT_AVAILABLE, &value);
        const auto it_to_erase_eventually = it;

        ++it;

        if (value == GL_TRUE /* || id == m_RootZone->puIds[0] || id == m_RootZone->puIds[1]*/) {
            GLuint64 value64 = 0;
            glGetQueryObjectui64v(id, GL_QUERY_RESULT, &value64);
            if (m_QueryIDToZone.find(id) != m_QueryIDToZone.end()) {
                auto ptr = m_QueryIDToZone[id];
                if (ptr != nullptr) {
                    if (id == ptr->puIds[0])
                        ptr->SetStartTimeStamp(value64);
                    else if (id == ptr->puIds[1])
                        ptr->SetEndTimeStamp(value64);
                    else
                        DEBUG_BREAK;
                }
            }
            m_PendingUpdate.erase(it_to_erase_eventually);
        } else {
            auto ptr = m_QueryIDToZone[id];
            if (ptr != nullptr) {
                LOG_ERROR_MESSAGE("%*s id not retrieved : %u", ptr->puDepth, "", id);
            }
        }
    }

#ifdef DEBUG_MODE_LOGGING
    DEBUG_MODE_LOGGING("------ End Frame -----");
#endif
}

void InAppGpuGLContext::DrawFlamGraph(const InAppGpuGraphTypeEnum& vGraphType) {
    if (m_RootZone != nullptr) {
        m_RootZone->DrawFlamGraph(vGraphType);
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

    if (InAppGpuScopedZone::sCurrentDepth == 0) {
#ifdef DEBUG_MODE_LOGGING
        DEBUG_MODE_LOGGING("------ Start Frame -----");
#endif
        m_DepthToLastZone.clear();
        if (m_RootZone == nullptr) {
            res = std::make_shared<InAppGpuQueryZone>(m_Context, vName, vSection, vIsRoot);
            if (res != nullptr) {
                res->puThis = res;
                res->puDepth = InAppGpuScopedZone::sCurrentDepth;
                m_QueryIDToZone[res->puIds[0]] = res;
                m_QueryIDToZone[res->puIds[1]] = res;
                m_RootZone = res;

#ifdef DEBUG_MODE_LOGGING
                DEBUG_MODE_LOGGING("Profile : add zone %s at puDepth %u", vName.c_str(), InAppGpuScopedZone::sCurrentDepth);
#endif
            }
        } else {
            res = m_RootZone;
        }
    } else {  // else child zone
        auto root = GetQueryZoneFromDepth(InAppGpuScopedZone::sCurrentDepth - 1U);
        if (root != nullptr) {
            if (root->puZonesDico.find(vName) == root->puZonesDico.end()) {  // not found
                res = InAppGpuQueryZone::create(m_Context, vName, vSection, vIsRoot);
                if (res != nullptr) {
                    res->puThis = res;
                    res->puDepth = InAppGpuScopedZone::sCurrentDepth;
                    m_QueryIDToZone[res->puIds[0]] = res;
                    m_QueryIDToZone[res->puIds[1]] = res;
                    root->puZonesDico[vName] = res;
                    root->puZonesOrdered.push_back(res);

#ifdef DEBUG_MODE_LOGGING
                    DEBUG_MODE_LOGGING("Profile : add zone %s at puDepth %u", vName.c_str(), InAppGpuScopedZone::sCurrentDepth);
#endif
                } else {
                    DEBUG_BREAK;
                }
            } else {
                res = root->puZonesDico[vName];
            }
        } else {
            return res;  // happen when profiling is activated inside a profiling zone
        }
    }

    /////////////////////////////////////////////
    //////////////// UTILISATION ////////////////
    /////////////////////////////////////////////

    if (res != nullptr) {
        SetQueryZoneForDepth(res, InAppGpuScopedZone::sCurrentDepth);

        if (res->puName != vName) {
#ifdef _DEBUG
            // at puDepth 0 there is only one frame
            LOG_DEBUG_ERROR_MESSAGE("was registerd at depth %u %s. but we got %s\nwe clear the profiler",  //
                                    InAppGpuScopedZone::sCurrentDepth, res->puName.c_str(), vName.c_str());
            // maybe the scoped frame is taken outside of the main frame
#endif
            Clear();
        }

        m_PendingUpdate.emplace(res->puIds[0]);
        m_PendingUpdate.emplace(res->puIds[1]);
    }

    return res;
}

void InAppGpuGLContext::SetQueryZoneForDepth(IAGPQueryZonePtr vInAppGpuQueryZone, GLuint vDepth) {
    m_DepthToLastZone[vDepth] = vInAppGpuQueryZone;
}

IAGPQueryZonePtr InAppGpuGLContext::GetQueryZoneFromDepth(GLuint vDepth) {
    IAGPQueryZonePtr res = nullptr;

    if (m_DepthToLastZone.find(vDepth) != m_DepthToLastZone.end()) {  // found
        res = m_DepthToLastZone[vDepth];
    }

    return res;
}

////////////////////////////////////////////////////////////
/////////////////////// GL PROFILER ////////////////////////
////////////////////////////////////////////////////////////

InAppGpuProfiler::InAppGpuProfiler() {
    Init();
};

InAppGpuProfiler::InAppGpuProfiler(const InAppGpuProfiler&) {
}

InAppGpuProfiler& InAppGpuProfiler::operator=(const InAppGpuProfiler&) {
    return *this;
};

InAppGpuProfiler::~InAppGpuProfiler() {
    Unit();
};

void InAppGpuProfiler::Clear() {
    m_Contexts.clear();
}

void InAppGpuProfiler::Init() {
}

void InAppGpuProfiler::Unit() {
    Clear();
}

void InAppGpuProfiler::Collect() {
    if (!puIsActive || puIsPaused) {
        return;
    }

    glFinish();

    for (const auto& con : m_Contexts) {
        if (con.second != nullptr) {
            con.second->Collect();
        }
    }
}

void InAppGpuProfiler::DrawFlamGraph() {
    if (!puIsActive) {
        return;
    }

    if (ImGui::BeginMenuBar()) {
        if (InAppGpuScopedZone::sMaxDepth) {
            InAppGpuQueryZone::sMaxDepthToOpen = InAppGpuScopedZone::sMaxDepth;
        }

        IMGUI_PLAY_PAUSE_BUTTON(puIsPaused);

        ImGui::Checkbox("Logging", &InAppGpuQueryZone::sActivateLogger);

        if (ImGui::BeginMenu("Graph Types")) {
            if (ImGui::MenuItem("Horizontal", nullptr, m_GraphType == iagp::InAppGpuGraphTypeEnum::IN_APP_GPU_HORIZONTAL)) {
                m_GraphType = iagp::InAppGpuGraphTypeEnum::IN_APP_GPU_HORIZONTAL;
            }
            if (ImGui::MenuItem("Circular", nullptr, m_GraphType == iagp::InAppGpuGraphTypeEnum::IN_APP_GPU_CIRCULAR)) {
                m_GraphType = iagp::InAppGpuGraphTypeEnum::IN_APP_GPU_CIRCULAR;
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    for (const auto& con : m_Contexts) {
        if (con.second != nullptr) {
            con.second->DrawFlamGraph(m_GraphType);
        }
    }
}

void InAppGpuProfiler::DrawDetails() {
    if (!puIsActive) {
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
    if (!puIsActive) {
        return nullptr;
    }

    if (vThreadPtr != nullptr) {
        if (m_Contexts.find((intptr_t)vThreadPtr) == m_Contexts.end()) {
            m_Contexts[(intptr_t)vThreadPtr] = std::make_shared<InAppGpuGLContext>(vThreadPtr);
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
    va_list args;
    va_start(args, fmt);
    static char TempBuffer[256];
    const int w = vsnprintf(TempBuffer, 256, fmt, args);
    va_end(args);
    if (w) {
        auto context_ptr = InAppGpuProfiler::Instance()->GetContextPtr(GET_CURRENT_CONTEXT());
        if (context_ptr != nullptr) {
            queryPtr = context_ptr->GetQueryZoneForName(std::string(TempBuffer, (size_t)w), vSection, vIsRoot);
            if (queryPtr != nullptr) {
                glQueryCounter(queryPtr->puIds[0], GL_TIMESTAMP);

#ifdef DEBUG_MODE_LOGGING
                DEBUG_MODE_LOGGING("%*s begin : %u", queryPtr->puDepth, "", queryPtr->puIds[0]);
#endif
                sCurrentDepth++;
            }
        }
    }
}

InAppGpuScopedZone::~InAppGpuScopedZone() {
    if (queryPtr != nullptr) {
        glQueryCounter(queryPtr->puIds[1], GL_TIMESTAMP);
        sCurrentDepth--;
    }
}

}  // namespace iagp
