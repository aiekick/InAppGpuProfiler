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

#include <LumoBackend/Profiler/vkInAppGpuProfiler.h>

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

#ifndef IAGP_SUB_WINDOW_MIN_SIZE
#define IAGP_SUB_WINDOW_MIN_SIZE ImVec2(300, 100)
#endif  // SUB_IAGP_WINDOW_MIN_SIZE

#ifndef IAGP_GPU_CONTEXT
#define IAGP_GPU_CONTEXT void*
#endif  // GPU_CONTEXT

#include <ctools/Logger.h>
#include <Gaia/gaia.h>
#include <Gaia/Core/VulkanCore.h>

#ifndef IAGP_IMGUI_BUTTON
#define IAGP_IMGUI_BUTTON ImGui ::Button
#endif  // IMGUI_BUTTON

#ifndef IAGP_IMGUI_PLAY_LABEL
#define IAGP_IMGUI_PLAY_LABEL "Play"
#endif

#ifndef IAGP_IMGUI_PAUSE_LABEL
#define IAGP_IMGUI_PAUSE_LABEL "Pause"
#endif

#ifndef IAGP_IMGUI_PLAY_PAUSE_HELP
#define IAGP_IMGUI_PLAY_PAUSE_HELP "Play/Pause Profiling"
#endif

#ifndef IAGP_IMGUI_PLAY_PAUSE_BUTTON
static bool PlayPauseButton(bool& vPlayPause) {
    bool res = false;
    const char* play_pause_label = IAGP_IMGUI_PAUSE_LABEL;
    if (vPlayPause) {
        play_pause_label = IAGP_IMGUI_PLAY_LABEL;
    }
    if (IAGP_IMGUI_BUTTON(play_pause_label)) {
        vPlayPause = !vPlayPause;
        res = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(IAGP_IMGUI_PLAY_PAUSE_HELP);
    }
    return res;
}
#define IAGP_IMGUI_PLAY_PAUSE_BUTTON PlayPauseButton
#endif  // LOG_DEBUG_ERROR_MESSAGE

#ifndef IAGP_DETAILS_TITLE
#define IAGP_DETAILS_TITLE "Profiler Details"
#endif  // IAGP_DETAILS_TITLE

namespace iagp {

void checkErrors(const char* vFile, const char* vFunc, const int& vLine) {
}

#define CheckErrors checkErrors(__FILE__, __FUNCTION__, __LINE__)

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

static bool PushStyleColorWithContrast(
    const ImU32& backGroundColor, const ImGuiCol& foreGroundColor, const ImVec4& invertedColor, const float& maxContrastRatio) {
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

uint32_t vkInAppGpuQueryZone::sMaxDepthToOpen = 100U;  // the max by default
bool vkInAppGpuQueryZone::sShowLeafMode = false;
float vkInAppGpuQueryZone::sContrastRatio = 4.3f;
bool vkInAppGpuQueryZone::sActivateLogger = false;
std::vector<IAGPQueryZoneWeak> vkInAppGpuQueryZone::sTabbedQueryZones = {};
IAGPQueryZonePtr vkInAppGpuQueryZone::create(
    IAGP_GPU_CONTEXT vContext, const void* vPtr, const std::string& vName, const std::string& vSectionName, const bool& vIsRoot) {
    auto res = std::make_shared<vkInAppGpuQueryZone>(vContext, vPtr, vName, vSectionName, vIsRoot);
    res->m_This = res;
    return res;
}
vkInAppGpuQueryZone::circularSettings vkInAppGpuQueryZone::sCircularSettings;

vkInAppGpuQueryZone::vkInAppGpuQueryZone(
    IAGP_GPU_CONTEXT vContext, const void* vPtr, const std::string& vName, const std::string& vSectionName, const bool& vIsRoot)
    : m_Context(vContext), name(vName), m_IsRoot(vIsRoot), m_SectionName(vSectionName), m_Ptr(vPtr) {
    m_StartFrameId = 0;
    m_EndFrameId = 0;
    m_StartTimeStamp = 0;
    m_EndTimeStamp = 0;
    m_ElapsedTime = 0.0;
    depth = vkInAppGpuScopedZone::sCurrentDepth;
    imGuiLabel = vName + "##vkInAppGpuQueryZone_" + std::to_string((intptr_t)this);

#ifdef OPENGL_PROFILING
    IAGP_SET_CURRENT_CONTEXT(m_Context);
    CheckErrors;
    glGenQueries(2, ids);
    CheckErrors;
#endif

}

vkInAppGpuQueryZone::~vkInAppGpuQueryZone() {
#ifdef OPENGL_PROFILING
    IAGP_SET_CURRENT_CONTEXT(m_Context);
    CheckErrors;
    glDeleteQueries(2, ids);
    CheckErrors;
#endif
    name.clear();
    m_StartFrameId = 0;
    m_EndFrameId = 0;
    m_StartTimeStamp = 0;
    m_EndTimeStamp = 0;
    m_ElapsedTime = 0.0;
    zonesOrdered.clear();
    zonesDico.clear();
}

void vkInAppGpuQueryZone::Clear() {
    m_StartFrameId = 0;
    m_EndFrameId = 0;
    m_StartTimeStamp = 0;
    m_EndTimeStamp = 0;
    m_ElapsedTime = 0.0;
}

void vkInAppGpuQueryZone::SetStartTimeStamp(const uint64_t& vValue) {
    m_StartTimeStamp = vValue;
    ++m_StartFrameId;
}

void vkInAppGpuQueryZone::SetEndTimeStamp(const uint64_t& vValue) {
    m_EndTimeStamp = vValue;
    ++m_EndFrameId;

    printf("%*s end id retrieved : %u\n", depth, "", ids[1]);

    // start computation of elapsed time
    // no needed after
    // will be used for Graph and labels
    // so DrawMetricGraph must be the first
    ComputeElapsedTime();

    if (vkInAppGpuQueryZone::sActivateLogger && zonesOrdered.empty())  // only the leafs
    {
        /*double v = (double)vValue / 1e9;
        LogVarLightInfo("<profiler section=\"%s\" epoch_time=\"%f\" name=\"%s\" render_time_ms=\"%f\">",
            m_SectionName.c_str(), v, name.c_str(), m_ElapsedTime);*/
    }
}

void vkInAppGpuQueryZone::ComputeElapsedTime() {
    // we take the last frame
    if (m_StartFrameId == m_EndFrameId) {
        m_AverageStartValue.AddValue(m_StartTimeStamp);  // ns to ms
        m_AverageEndValue.AddValue(m_EndTimeStamp);      // ns to ms
        m_StartTime = (double)(m_AverageStartValue.GetAverage() * 1e-6);
        m_EndTime = (double)(m_AverageEndValue.GetAverage() * 1e-6);
        m_ElapsedTime = m_EndTime - m_StartTime;
    }
}

void vkInAppGpuQueryZone::DrawDetails() {
    if (m_StartFrameId) {
        bool res = false;

        ImGui::TableNextColumn();  // tree

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_CollapsingHeader | ImGuiTreeNodeFlags_DefaultOpen;

        bool any_childs_to_show = false;
        for (const auto zone : zonesOrdered) {
            if (zone != nullptr && zone->m_ElapsedTime > 0.0) {
                any_childs_to_show = true;
                break;
            }
        }

        if (!any_childs_to_show) {
            flags |= ImGuiTreeNodeFlags_Leaf;
        }

        if (m_Highlighted) {
            flags |= ImGuiTreeNodeFlags_Framed;
        }

        const auto colorU32 = ImGui::ColorConvertFloat4ToU32(cv4);
        const bool pushed = PushStyleColorWithContrast(colorU32, ImGuiCol_Text, ImVec4(0, 0, 0, 1), vkInAppGpuQueryZone::sContrastRatio);

        ImGui::PushStyleColor(ImGuiCol_Header, cv4);
        const auto hovered_color = ImVec4(cv4.x * 0.9f, cv4.y * 0.9f, cv4.z * 0.9f, 1.0f);
        const auto active_color = ImVec4(cv4.x * 0.8f, cv4.y * 0.8f, cv4.z * 0.8f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, hovered_color);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, active_color);

        if (m_IsRoot) {
            res = ImGui::TreeNodeEx(this, flags, "%s : frame [%u]", name.c_str(), m_StartFrameId - 1U);
        } else if (!m_SectionName.empty()) {
            res = ImGui::TreeNodeEx(this, flags, "%s : %s", m_SectionName.c_str(), name.c_str());
        } else {
            res = ImGui::TreeNodeEx(this, flags, "%s", name.c_str());
        }

        ImGui::PopStyleColor(3);

        if (pushed) {
            ImGui::PopStyleColor();
        }

        if (ImGui::IsItemHovered()) {
            m_Highlighted = true;
        }

#ifdef IAGP_SHOW_COUNT
        ImGui::TableNextColumn();  // Elapsed time
        ImGui::Text("%u", last_count);
#endif
        ImGui::TableNextColumn();  // Elapsed time
        ImGui::Text("%.5f ms", m_ElapsedTime);
        ImGui::TableNextColumn();  // Max fps
        if (m_ElapsedTime > 0.0f) {
            ImGui::Text("%.2f f/s", 1000.0f / m_ElapsedTime);
        } else {
            ImGui::Text("%s", "Infinite");
        }
        ImGui::TableNextColumn();  // start time
        ImGui::Text("%.5f ms", m_StartTime);
        ImGui::TableNextColumn();  // end time
        ImGui::Text("%.5f", m_EndTime);

        if (res) {
            m_Expanded = true;
            ImGui::Indent();
            for (const auto zone : zonesOrdered) {
                if (zone != nullptr && zone->m_ElapsedTime > 0.0) {
                    zone->DrawDetails();
                }
            }
            ImGui::Unindent();
        } else {
            m_Expanded = false;
        }
    }
}

bool vkInAppGpuQueryZone::DrawFlamGraph(
    vkInAppGpuGraphTypeEnum vGraphType, IAGPQueryZoneWeak& vOutSelectedQuery, IAGPQueryZoneWeak vParent, uint32_t vDepth) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) {
        return false;
    }

    bool pressed = false;
    switch (vGraphType) {
        case vkInAppGpuGraphTypeEnum::IN_APP_GPU_HORIZONTAL:  // horizontal flame graph (standard and legacy)
            pressed = m_DrawHorizontalFlameGraph(m_This.lock(), vOutSelectedQuery, vParent, vDepth);
            break;
        case vkInAppGpuGraphTypeEnum::IN_APP_GPU_CIRCULAR:  // circular flame graph
            pressed = m_DrawCircularFlameGraph(m_This.lock(), vOutSelectedQuery, vParent, vDepth);
            break;
    }
    return pressed;
}

void vkInAppGpuQueryZone::UpdateBreadCrumbTrail() {
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
        for (uint32_t idx = 0U; idx < depth; ++idx) {
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
        imGuiTitle += "##vkInAppGpuQueryZone_ " + std::to_string((intptr_t)this);
    }
}

void vkInAppGpuQueryZone::DrawBreadCrumbTrail(IAGPQueryZoneWeak& vOutSelectedQuery) {
    ImGui::PushID("DrawBreadCrumbTrail");
    for (uint32_t idx = 0U; idx < depth; ++idx) {
        if (idx < m_BreadCrumbTrail.size()) {
            auto ptr = m_BreadCrumbTrail[idx].lock();
            if (ptr != nullptr) {
                if (idx > 0U) {
                    ImGui::SameLine();
                    ImGui::Text("%s", ">");
                    ImGui::SameLine();
                }
                ImGui::PushID(ptr.get());
                if (IAGP_IMGUI_BUTTON(ptr->imGuiLabel.c_str())) {
                    vOutSelectedQuery = m_BreadCrumbTrail[idx];
                }
                ImGui::PopID();
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

void vkInAppGpuQueryZone::m_DrawList_DrawBar(const char* vLabel, const ImRect& vRect, const ImVec4& vColor, const bool& vHovered) {
    const ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    const ImGuiStyle& style = g.Style;
    const ImVec2 label_size = ImGui::CalcTextSize(vLabel, nullptr, true);

    const auto colorU32 = ImGui::ColorConvertFloat4ToU32(vColor);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::RenderFrame(vRect.Min, vRect.Max, colorU32, true, 2.0f);
    if (vHovered) {
        const auto selectU32 = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f - cv4.x, 1.0f - cv4.y, 1.0f - cv4.z, 1.0f));
        window->DrawList->AddRect(vRect.Min, vRect.Max, selectU32, true, 0, 2.0f);
    }
    ImGui::PopStyleVar();

    const bool pushed = PushStyleColorWithContrast(colorU32, ImGuiCol_Text, ImVec4(0, 0, 0, 1), vkInAppGpuQueryZone::sContrastRatio);
    ImGui::RenderTextClipped(vRect.Min + style.FramePadding, vRect.Max - style.FramePadding,  //
        vLabel, nullptr, &label_size, ImVec2(0.5f, 0.5f), &vRect);
    if (pushed) {
        ImGui::PopStyleColor();
    }
}

bool vkInAppGpuQueryZone::m_ComputeRatios(
    IAGPQueryZonePtr vRoot, IAGPQueryZoneWeak vParent, uint32_t vDepth, float& vOutStartRatio, float& vOutSizeRatio) {
    if (depth > vkInAppGpuQueryZone::sMaxDepthToOpen) {
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
            if (rootPtr == nullptr) {
                hsv = ImVec4((float)(0.5 - 0.5 * m_ElapsedTime / vRoot->m_ElapsedTime), 0.5f, 1.0f, 1.0f);
            } else {
                hsv = ImVec4((float)(0.5 - 0.5 * m_ElapsedTime / rootPtr->m_ElapsedTime), 0.5f, 1.0f, 1.0f);
            }
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
                    if (rootPtr == nullptr) {
                        hsv = ImVec4((float)(0.5 - 0.5 * m_ElapsedTime / vRoot->m_ElapsedTime), 0.5f, 1.0f, 1.0f);
                    } else {
                        hsv = ImVec4((float)(0.5 - 0.5 * m_ElapsedTime / rootPtr->m_ElapsedTime), 0.5f, 1.0f, 1.0f);
                    }
                }
            }
        }
        return true;
    }
    return false;
}

bool vkInAppGpuQueryZone::m_DrawHorizontalFlameGraph(
    IAGPQueryZonePtr vRoot, IAGPQueryZoneWeak& vOutSelectedQuery, IAGPQueryZoneWeak vParent, uint32_t vDepth) {
    bool pressed = false;
    const ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const float aw = ImGui::GetContentRegionAvail().x - style.FramePadding.x;
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    float barStartRatio = 0.0f;
    float barSizeRatio = 0.0f;
    if (m_ComputeRatios(vRoot, vParent, vDepth, barStartRatio, barSizeRatio)) {
        if (barSizeRatio > 0.0f) {
            if ((zonesOrdered.empty() && vkInAppGpuQueryZone::sShowLeafMode) || !vkInAppGpuQueryZone::sShowLeafMode) {
                ImGui::PushID(this);
                m_BarLabel = toStr("%s (%.2f ms | %.2f f/s)", name.c_str(), m_ElapsedTime, 1000.0f / m_ElapsedTime);
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
                pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held,  //
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
                    ImGui::SetTooltip("Section : [%s : %s]\nElapsed time : %.5f ms\nElapsed FPS : %.5f f/s",  //
                        m_SectionName.c_str(), name.c_str(), m_ElapsedTime, 1000.0f / m_ElapsedTime);
                    m_Highlighted = true;  // to highlight label graph by this button
                } else if (m_Highlighted) {
                    hovered = true;  // highlight this button by the label graph
                }
                ImGui::ColorConvertHSVtoRGB(hsv.x, hsv.y, hsv.z, cv4.x, cv4.y, cv4.z);
                cv4.w = 1.0f;
                ImGui::RenderNavHighlight(bb, id);
                m_DrawList_DrawBar(label, bb, cv4, hovered);
                ++vDepth;
            }

            // we dont show child if this one have elapsed time to 0.0
            for (const auto zone : zonesOrdered) {
                if (zone != nullptr) {
                    pressed |= zone->m_DrawHorizontalFlameGraph(vRoot, vOutSelectedQuery, m_This, vDepth);
                }
            }
        } else {
#ifdef printf
            printf("Bar Ms not displayed", name.c_str());
#endif
        }
    }

    if (depth == 0 && ((zonesOrdered.empty() && vkInAppGpuQueryZone::sShowLeafMode) || !vkInAppGpuQueryZone::sShowLeafMode)) {
        const ImVec2 pos = window->DC.CursorPos;
        const ImVec2 size = ImVec2(aw, ImGui::GetFrameHeight() * (vkInAppGpuScopedZone::sMaxDepth + 1U));
        ImGui::ItemSize(size);
        const ImRect bb(pos, pos + size);
        const ImGuiID id = window->GetID((name + "##canvas").c_str());
        if (!ImGui::ItemAdd(bb, id)) {
            return pressed;
        }
    }

    return pressed;
}

bool vkInAppGpuQueryZone::m_DrawCircularFlameGraph(
    IAGPQueryZonePtr vRoot, IAGPQueryZoneWeak& vOutSelectedQuery, IAGPQueryZoneWeak vParent, uint32_t vDepth) {
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
            if ((zonesOrdered.empty() && vkInAppGpuQueryZone::sShowLeafMode) || !vkInAppGpuQueryZone::sShowLeafMode) {
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
/////////////////////// 3D CONTEXT /////////////////////////
////////////////////////////////////////////////////////////

IAGPContextPtr vkInAppGpuContext::create(IAGP_GPU_CONTEXT vContext) {
    auto res = std::make_shared<vkInAppGpuContext>(vContext);
    res->m_This = res;
    return res;
}

vkInAppGpuContext::vkInAppGpuContext(IAGP_GPU_CONTEXT vContext) : m_Context(vContext) {

}

void vkInAppGpuContext::Clear() {
    m_RootZone.reset();
#ifdef OPENGl_PROFILING
    m_PendingUpdate.clear();
#endif
    m_QueryIDToZone.clear();
    m_DepthToLastZone.clear();
    m_TimeStampMeasures.clear();
}

#ifdef VULKAN_PROFILING
bool vkInAppGpuContext::Init(VkPhysicalDevice vPhysicalDevice, VkDevice vLogicalDevice, const uint32_t& vMaxQueryCount) {
    m_PhysicalDevice = vPhysicalDevice;
    m_LogicalDevice = vLogicalDevice;

    VkPhysicalDeviceProperties props;
    VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceProperties(m_PhysicalDevice, &props);
    m_TimeStampPeriod = props.limits.timestampPeriod;

    m_QueryCount = vMaxQueryCount;  // base count

    VkQueryPoolCreateInfo poolInfos = {};
    poolInfos.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    poolInfos.queryCount = (uint32_t)m_QueryCount;
    poolInfos.queryType = VK_QUERY_TYPE_TIMESTAMP;

    VkResult creation_result = VkResult::VK_NOT_READY;
    while ((creation_result = VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateQueryPool(m_LogicalDevice, &poolInfos, nullptr, &m_QueryPool)) != VK_SUCCESS) {
        m_QueryCount /= 2U;
        poolInfos.queryCount = (uint32_t)m_QueryCount;
    }

    m_QueryTail = 0U;
    m_QueryHead = 0U;

    return (creation_result == VK_SUCCESS);
}
#endif

void vkInAppGpuContext::Unit() {
    Clear();
#ifdef VULKAN_PROFILING
    VULKAN_HPP_DEFAULT_DISPATCHER.vkDeviceWaitIdle(m_LogicalDevice);
    VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyQueryPool(m_LogicalDevice, m_QueryPool, nullptr);
#endif
}

void vkInAppGpuContext::Collect(
#ifdef VULKAN_PROFILING
    VkCommandBuffer vCmd
#endif
) {
    printf("------ Collect Thread (%u) -----\n", (uint32_t)(intptr_t)m_Context);

#ifdef VULKAN_PROFILING
    if (!m_TimeStampMeasures.empty() && 
        VULKAN_HPP_DEFAULT_DISPATCHER.vkGetQueryPoolResults(m_LogicalDevice, m_QueryPool, (uint32_t)m_QueryTail,
            (uint32_t)m_QueryHead,
            sizeof(vkTimeStamp) * m_TimeStampMeasures.size(),
            m_TimeStampMeasures.data(), sizeof(vkTimeStamp), VK_QUERY_RESULT_64_BIT /* | VK_QUERY_RESULT_WAIT_BIT*/) == VK_NOT_READY) {
        return;
    }

    VULKAN_HPP_DEFAULT_DISPATCHER.vkCmdResetQueryPool(vCmd, m_QueryPool, 0U, (uint32_t)m_QueryCount);

    for (size_t id = 0; id < m_TimeStampMeasures.size(); ++id) {
        auto query_it = m_QueryIDToZone.find((uint32_t)id);
        if (query_it != m_QueryIDToZone.end()) {
            vkTimeStamp value64 = m_TimeStampMeasures[id];
            auto ptr = query_it->second;
            if (ptr != nullptr) {
                if (id == ptr->ids[0]) {
                    ptr->SetStartTimeStamp(value64);
                } else if (id == ptr->ids[1]) {
                    ptr->last_count = ptr->current_count;
                    ptr->current_count = 0U;
                    ptr->SetEndTimeStamp(value64);
                } else {
                    DEBUG_BREAK;
                }
            }
        }
    }
#endif

#ifdef OPENGl_PROFILING
    auto it = m_PendingUpdate.begin();
    while (!m_PendingUpdate.empty() && it != m_PendingUpdate.end()) {
        uint32_t id = *it;
        uint32_t value = 0;
        glGetQueryObjectuiv(id, GL_QUERY_RESULT_AVAILABLE, &value);
        const auto it_to_erase_eventually = it;
        ++it;
        if (value == 1) {
            uint64_t value64 = 0;
            glGetQueryObjectui64v(id, GL_QUERY_RESULT, &value64);
            if (m_QueryIDToZone.find(id) != m_QueryIDToZone.end()) {
                auto ptr = m_QueryIDToZone[id];
                if (ptr != nullptr) {
                    if (id == ptr->ids[0]) {
                        ptr->SetStartTimeStamp(value64);
                    } else if (id == ptr->ids[1]) {
                        ptr->last_count = ptr->current_count;
                        ptr->current_count = 0U;
                        ptr->SetEndTimeStamp(value64);
                    } else {
                        DEBUG_BREAK;
                    }
                }
            }
            m_PendingUpdate.erase(it_to_erase_eventually);
        } else {
            auto ptr = m_QueryIDToZone[id];
            if (ptr != nullptr) {
                printf("%*s id not retrieved : %u", ptr->depth, "", id);
            }
        }
    }
#endif

    printf("------ End Frame -----\n");
}

void vkInAppGpuContext::DrawFlamGraph(const vkInAppGpuGraphTypeEnum& vGraphType) {
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

void vkInAppGpuContext::DrawDetails() {
    if (m_RootZone != nullptr) {
        m_RootZone->DrawDetails();
    }
}

IAGPQueryZonePtr vkInAppGpuContext::GetQueryZoneForName(
    const void* vPtr, const std::string& vName, const std::string& vSection, const bool& vIsRoot) {
    IAGPQueryZonePtr res = nullptr;

    //////////////// CREATION ///////////////////

    // there is many link issues with 'max' in cross compilation so we dont using it
    if (vkInAppGpuScopedZone::sCurrentDepth > vkInAppGpuScopedZone::sMaxDepth) {
        vkInAppGpuScopedZone::sMaxDepth = vkInAppGpuScopedZone::sCurrentDepth;
    }

    if (vkInAppGpuScopedZone::sCurrentDepth == 0) {  // root zone
        printf("------ Start Frame -----\n");
        m_DepthToLastZone.clear();
        if (m_RootZone == nullptr) {
            res = vkInAppGpuQueryZone::create(m_Context, vPtr, vName, vSection, vIsRoot);
            if (res != nullptr) {
#ifdef VULKAN_PROFILING
                res->ids[0] = GetNextQueryId();
                res->ids[1] = GetNextQueryId();
#endif
                res->depth = vkInAppGpuScopedZone::sCurrentDepth;
                res->UpdateBreadCrumbTrail();
                m_QueryIDToZone[res->ids[0]] = res;
                m_QueryIDToZone[res->ids[1]] = res;
                m_RootZone = res;

                // printf("Profile : add zone %s at puDepth %u", vName.c_str(), vkInAppGpuScopedZone::sCurrentDepth);
            }
        } else {
            res = m_RootZone;
        }
    } else {  // else child zone
        auto root = m_GetQueryZoneFromDepth(vkInAppGpuScopedZone::sCurrentDepth - 1U);
        if (root != nullptr) {
            bool found = false;
            const auto& key_str = vSection + vName;
            const auto& ptr_iter = root->zonesDico.find(vPtr);
            if (ptr_iter == root->zonesDico.end()) {  // not found
                found = false;
            } else {                                                                 // found
                found = (ptr_iter->second.find(key_str) != ptr_iter->second.end());  // not found
            }
            if (!found) {  // not found
                res = vkInAppGpuQueryZone::create(m_Context, vPtr, vName, vSection, vIsRoot);
                if (res != nullptr) {
#ifdef VULKAN_PROFILING
                    res->ids[0] = GetNextQueryId();
                    res->ids[1] = GetNextQueryId();
#endif
                    res->parentPtr = root;
                    res->rootPtr = m_RootZone;
                    res->depth = vkInAppGpuScopedZone::sCurrentDepth;
                    res->UpdateBreadCrumbTrail();
                    m_QueryIDToZone[res->ids[0]] = res;
                    m_QueryIDToZone[res->ids[1]] = res;
                    root->zonesDico[vPtr][key_str] = res;
                    root->zonesOrdered.push_back(res);
                    // printf("Profile : add zone %s at puDepth %u", vName.c_str(), vkInAppGpuScopedZone::sCurrentDepth);
                } else {
                    DEBUG_BREAK;
                }
            } else {
                res = root->zonesDico[vPtr][key_str];
            }
        } else {
            return res;  // happen when profiling is activated inside a profiling zone
        }
    }

    //////////////// UTILISATION ////////////////

    if (res != nullptr) {
        m_SetQueryZoneForDepth(res, vkInAppGpuScopedZone::sCurrentDepth);
        if (res->name != vName) {
            // at depth 0 there is only one frame
            printf("was registerd at depth %u %s. but we got %s\nwe clear the profiler",  //
                vkInAppGpuScopedZone::sCurrentDepth, res->name.c_str(), vName.c_str());
            // maybe the scoped frame is taken outside of the main frame
            Clear();
        }
#ifdef OPENGl_PROFILING
        m_PendingUpdate.emplace(res->ids[0]);
        m_PendingUpdate.emplace(res->ids[1]);
#endif
    }

    return res;
}

void vkInAppGpuContext::m_SetQueryZoneForDepth(IAGPQueryZonePtr vvkInAppGpuQueryZone, uint32_t vDepth) {
    m_DepthToLastZone[vDepth] = vvkInAppGpuQueryZone;
}

IAGPQueryZonePtr vkInAppGpuContext::m_GetQueryZoneFromDepth(uint32_t vDepth) {
    IAGPQueryZonePtr res = nullptr;

    if (m_DepthToLastZone.find(vDepth) != m_DepthToLastZone.end()) {  // found
        res = m_DepthToLastZone[vDepth];
    }

    return res;
}

#ifdef VULKAN_PROFILING
int32_t vkInAppGpuContext::GetNextQueryId() {
    const auto id = m_QueryHead;
    m_QueryHead = (m_QueryHead + 1) % m_QueryCount;
    assert(m_QueryHead != m_QueryTail);
    m_TimeStampMeasures.push_back(0); // append a value
    return (uint32_t)id;
}

VkQueryPool vkInAppGpuContext::GetQueryPool() {
    return m_QueryPool;
}
#endif

////////////////////////////////////////////////////////////
/////////////////////// SCOPED ZONE ////////////////////////
////////////////////////////////////////////////////////////

// STATIC
uint32_t vkInAppGpuScopedZone::sCurrentDepth = 0U;
uint32_t vkInAppGpuScopedZone::sMaxDepth = 0U;

// SCOPED ZONE
vkInAppGpuScopedZone::vkInAppGpuScopedZone(  //
    const VkCommandBuffer& vCmd,             //
    const bool& vIsRoot,                     //
    void* vContextPtr,                       //
    const void* vPtr,                        //
    const std::string& vSection,             //
    const char* fmt,                         //
    ...) {                                   //
    if (vkInAppGpuProfiler::sIsActive) {
        va_list args;
        va_start(args, fmt);
        static char TempBuffer[256];
        const int w = vsnprintf(TempBuffer, 256, fmt, args);
        va_end(args);
        if (w) {
            auto context_ptr = vkInAppGpuProfiler::Instance()->GetContextPtr(vContextPtr);
            if (context_ptr != nullptr) {
                const auto& label = std::string(TempBuffer, (size_t)w);
                queryPtr = context_ptr->GetQueryZoneForName(vPtr, label, vSection, vIsRoot);
                if (queryPtr != nullptr) {
#ifdef VULKAN_PROFILING
                    queryPtr->commandBuffer = vCmd;
                    queryPtr->queryPool = context_ptr->GetQueryPool();
                    VULKAN_HPP_DEFAULT_DISPATCHER.vkCmdWriteTimestamp(
                        queryPtr->commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPtr->queryPool, queryPtr->ids[0]);
#endif
                    printf("%*s begin : [%u:%u] (depth:%u) (%s)",  //
                        queryPtr->depth, "", queryPtr->ids[0], queryPtr->ids[1], queryPtr->depth, label.c_str());
                    sCurrentDepth++;
                }
            }
        }
    }
}

vkInAppGpuScopedZone::~vkInAppGpuScopedZone() {
    if (vkInAppGpuProfiler::sIsActive) {
        if (queryPtr != nullptr) {
            if (queryPtr->depth > 0) {
                printf("%*s end : [%u:%u] (depth:%u)",  //
                    (queryPtr->depth - 1U), "", queryPtr->ids[0], queryPtr->ids[1], queryPtr->depth);
            } else {
                printf("end : [%u:%u] (depth:%u)",  //
                    queryPtr->ids[0], queryPtr->ids[1], 0);
            }
#ifdef VULKAN_PROFILING
            VULKAN_HPP_DEFAULT_DISPATCHER.vkCmdWriteTimestamp(
                queryPtr->commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPtr->queryPool, queryPtr->ids[1]);
#endif
            ++queryPtr->current_count;
            --sCurrentDepth;
        }
    }
}

////////////////////////////////////////////////////////////
/////////////////////// 3D PROFILER ////////////////////////
////////////////////////////////////////////////////////////

bool vkInAppGpuProfiler::sIsActive = false;
bool vkInAppGpuProfiler::sIsPaused = false;

vkInAppGpuProfiler::vkInAppGpuProfiler() = default;
vkInAppGpuProfiler::vkInAppGpuProfiler(const vkInAppGpuProfiler&) = default;

vkInAppGpuProfiler& vkInAppGpuProfiler::operator=(const vkInAppGpuProfiler&) {
    return *this;
};

vkInAppGpuProfiler::~vkInAppGpuProfiler() {
    Destroy();
};

bool vkInAppGpuProfiler::addContext(void* vContextPtr, VkPhysicalDevice vPhysicalDevice, VkDevice vLogicalDevice, const uint32_t& vMaxQueryCount) {
    bool res = false;
    auto ctx_ptr = GetContextPtr(vContextPtr);
    if (ctx_ptr != nullptr) {
        res = ctx_ptr->Init(vPhysicalDevice, vLogicalDevice, vMaxQueryCount);
    }
    if (!res) {
        m_Contexts.erase((intptr_t)vContextPtr);
    }
    return res;
}

void vkInAppGpuProfiler::Destroy() {
    m_Contexts.clear();
}

void vkInAppGpuProfiler::Collect(
#ifdef VULKAN_PROFILING
        VkCommandBuffer vResetQueryPoolCmd
#endif
) {
    if (!sIsActive || sIsPaused) {
        return;
    }

    for (const auto& con : m_Contexts) {
        if (con.second != nullptr) {
            con.second->Collect(
#ifdef VULKAN_PROFILING
        vResetQueryPoolCmd
#endif
            );
        }
    }
}

void vkInAppGpuProfiler::DrawFlamGraph(const char* vLabel, bool* pOpen, ImGuiWindowFlags vFlags) {
    if (m_ImGuiBeginFunctor != nullptr && m_ImGuiBeginFunctor(vLabel, pOpen, vFlags | ImGuiWindowFlags_MenuBar)) {
        DrawFlamGraphNoWin();
    }
    if (m_ImGuiEndFunctor != nullptr) {
        m_ImGuiEndFunctor();
    }

    DrawFlamGraphChilds(vFlags);

    DrawDetails(vFlags);
}

void vkInAppGpuProfiler::DrawFlamGraphNoWin() {
    if (sIsActive) {
        m_DrawMenuBar();
        for (const auto& con : m_Contexts) {
            if (con.second != nullptr) {
                con.second->DrawFlamGraph(m_GraphType);
            }
        }
    }
}

void vkInAppGpuProfiler::DrawFlamGraphChilds(ImGuiWindowFlags vFlags) {
    m_SelectedQuery.reset();
    m_QueryZoneToClose = -1;
    for (size_t idx = 0U; idx < iagp::vkInAppGpuQueryZone::sTabbedQueryZones.size(); ++idx) {
        auto ptr = iagp::vkInAppGpuQueryZone::sTabbedQueryZones[idx].lock();
        if (ptr != nullptr) {
            bool opened = true;
            ImGui::SetNextWindowSizeConstraints(IAGP_SUB_WINDOW_MIN_SIZE, ImGui::GetIO().DisplaySize);
            if (m_ImGuiBeginFunctor != nullptr && m_ImGuiBeginFunctor(ptr->imGuiTitle.c_str(), &opened, vFlags)) {
                if (sIsActive) {
                    ptr->DrawFlamGraph(iagp::vkInAppGpuProfiler::Instance()->GetGraphTypeRef(), m_SelectedQuery);
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
        iagp::vkInAppGpuQueryZone::sTabbedQueryZones.erase(  //
            iagp::vkInAppGpuQueryZone::sTabbedQueryZones.begin() + m_QueryZoneToClose);
    }
}

void vkInAppGpuProfiler::SetImGuiBeginFunctor(const ImGuiBeginFunctor& vImGuiBeginFunctor) {
    m_ImGuiBeginFunctor = vImGuiBeginFunctor;
}

void vkInAppGpuProfiler::SetImGuiEndFunctor(const ImGuiEndFunctor& vImGuiEndFunctor) {
    m_ImGuiEndFunctor = vImGuiEndFunctor;
}

void vkInAppGpuProfiler::m_DrawMenuBar() {
    if (ImGui::BeginMenuBar()) {
        if (vkInAppGpuScopedZone::sMaxDepth) {
            vkInAppGpuQueryZone::sMaxDepthToOpen = vkInAppGpuScopedZone::sMaxDepth;
        }

        IAGP_IMGUI_PLAY_PAUSE_BUTTON(sIsPaused);

        if (IAGP_IMGUI_BUTTON("Details")) {
            m_ShowDetails = !m_ShowDetails;
        }

#ifdef IAGP_DEV_MODE
        ImGui::Checkbox("Logging", &vkInAppGpuQueryZone::sActivateLogger);

        if (ImGui::BeginMenu("Graph Types")) {
            if (ImGui::MenuItem("Horizontal", nullptr, m_GraphType == iagp::vkInAppGpuGraphTypeEnum::IN_APP_GPU_HORIZONTAL)) {
                m_GraphType = iagp::vkInAppGpuGraphTypeEnum::IN_APP_GPU_HORIZONTAL;
            }
            if (ImGui::MenuItem("Circular", nullptr, m_GraphType == iagp::vkInAppGpuGraphTypeEnum::IN_APP_GPU_CIRCULAR)) {
                m_GraphType = iagp::vkInAppGpuGraphTypeEnum::IN_APP_GPU_CIRCULAR;
            }
            ImGui::EndMenu();
        }
#endif

        ImGui::EndMenuBar();
    }
}

void vkInAppGpuProfiler::DrawDetails(ImGuiWindowFlags vFlags) {
    if (m_ShowDetails) {
        if (m_ImGuiBeginFunctor != nullptr && m_ImGuiBeginFunctor(IAGP_DETAILS_TITLE, &m_ShowDetails, vFlags)) {
            DrawDetailsNoWin();
        }
        if (m_ImGuiEndFunctor != nullptr) {
            m_ImGuiEndFunctor();
        }
    }
}

void vkInAppGpuProfiler::DrawDetailsNoWin() {
    if (!sIsActive) {
        return;
    }

    int32_t count_tables = 5;
#ifdef IAGP_SHOW_COUNT
    ++count_tables;
#endif

    static ImGuiTableFlags flags =        //
        ImGuiTableFlags_SizingFixedFit |  //
        ImGuiTableFlags_RowBg |           //
        ImGuiTableFlags_Hideable |        //
        ImGuiTableFlags_ScrollY |         //
        ImGuiTableFlags_NoHostExtendY;
    const auto& size = ImGui::GetContentRegionAvail();
    auto listViewID = ImGui::GetID("##vkInAppGpuProfiler_DrawDetails");
    if (ImGui::BeginTableEx("##vkInAppGpuProfiler_DrawDetails", listViewID, count_tables, flags, size, 0.0f)) {
        ImGui::TableSetupColumn("Tree", ImGuiTableColumnFlags_WidthStretch);
#ifdef IAGP_SHOW_COUNT
        ImGui::TableSetupColumn("Count");
#endif
        ImGui::TableSetupColumn("Elapsed time");
        ImGui::TableSetupColumn("Max fps");
        ImGui::TableSetupColumn("Start time");
        ImGui::TableSetupColumn("End time");
        ImGui::TableHeadersRow();
        for (const auto& con : m_Contexts) {
            if (con.second != nullptr) {
                con.second->DrawDetails();
            }
        }
        ImGui::EndTable();
    }
}

IAGPContextPtr vkInAppGpuProfiler::GetContextPtr(IAGP_GPU_CONTEXT vThreadPtr) {
    if (m_Contexts.find((intptr_t)vThreadPtr) == m_Contexts.end()) {
        m_Contexts[(intptr_t)vThreadPtr] = vkInAppGpuContext::create(vThreadPtr);
    }
    return m_Contexts[(intptr_t)vThreadPtr];
}

}  // namespace iagp
