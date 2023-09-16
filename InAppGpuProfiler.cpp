// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2017-2023 Stephane Cuillerdier All rights reserved

// This is an independent m_oject of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "InAppGpuProfiler.h"

#include <cstdarg> /* va_list, va_start, va_arg, va_end */

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif  // IMGUI_DEFINE_MATH_OPERATORS

#include <imgui_internal.h>

#ifdef _MSC_VER
#define DEBUG_BREAK          \
    if (IsDebuggerPresent()) \
    __debugbreak()
#else
#define DEBUG_BREAK
#endif

namespace iagp {
static void SetCurrentContext(GPU_CONTEXT_PTR vContextPtr) {
}

static GPU_CONTEXT_PTR GetCurrentContext() {
    return nullptr;
}
static void LogError(const char* fmt, ...) {
}
static void LogDebugError(const char* fmt, ...) {
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
static void RenderTextClipped(const ImVec2& pos_min, const ImVec2& pos_max, const char* text, const char* text_end, const ImVec2* text_size_if_known,
                              const ImVec2& align, const ImRect* clip_rect) {
    ImGui::RenderTextClipped(pos_min, pos_max, text, text_end, text_size_if_known, align, clip_rect);
}

class InAppGpuAverageValue {
private:
    double m_PerFrame[60];
    int m_PerFrameIdx = 0;
    double m_PerFrameAccum = 0.0;
    double m_AverageValue = 0.0;

public:
    InAppGpuAverageValue();
    void AddValue(double vValue);
    double GetAverage();
};

class InAppGpuQueryZone {
public:
    static GLuint sMaxDepthToOpen;
    static bool sShowLeafMode;
    static float sContrastRatio;
    static bool sActivateLogger;

public:
    GLuint puDepth = 0U;  // the puDepth of the QueryZone
    GLuint puIds[2] = {0U, 0U};
    std::vector<IAGPQueryZonePtr> puZonesOrdered;
    std::unordered_map<std::string, IAGPQueryZonePtr> puZonesDico;  // main container
    std::string puName;
    IAGPQueryZonePtr puThis = nullptr;
    IAGPQueryZonePtr puTopQuery = nullptr;

private:
    bool m_IsRoot = false;
    double m_ElapsedTime = 0.0;
    double m_StartTime = 0.0;
    double m_EndTime = 0.0;
    GLuint m_StartFrameId = 0;
    GLuint m_EndFrameId = 0;
    GLuint64 m_StartTimeStamp = 0;
    GLuint64 m_EndTimeStamp = 0;
    bool m_Expanded = false;
    bool m_Highlighted = false;
    InAppGpuAverageValue m_AverageStartValue;
    InAppGpuAverageValue m_AverageEndValue;
    GPU_CONTEXT_PTR m_ThreadPtr = nullptr;
    std::string m_BarLabel;
    std::string m_SectionName;
    float m_BarWidth = 0.0f;
    float m_BarPos = 0.0f;

public:
    InAppGpuQueryZone(GPU_CONTEXT_PTR vThread, const std::string& vName, const std::string& vSectionName, const bool& vIsRoot = false);
    ~InAppGpuQueryZone();
    void Clear();
    void SetStartTimeStamp(const GLuint64& vValue);
    void SetEndTimeStamp(const GLuint64& vValue);
    void ComputeElapsedTime();
    void DrawMetricLabels();
    bool DrawMetricGraph(IAGPQueryZonePtr vParent = nullptr, uint32_t vDepth = 0);
};

class InAppGpuGLContext {
private:
    GPU_CONTEXT_PTR m_ThreadPtr;
    IAGPQueryZonePtr m_RootZone = nullptr;
    std::unordered_map<GLuint, IAGPQueryZonePtr>
        m_QueryIDToZone;  // Get the zone for a query id because a query have to id's : start and end
    std::unordered_map<GLuint, IAGPQueryZonePtr> m_DepthToLastZone;  // last zone registered at this puDepth
    std::set<GLuint> m_PendingUpdate;                                                  // some queries msut but retrieveds

public:
    InAppGpuGLContext(GPU_CONTEXT_PTR vThread);
    void Clear();
    void Init();
    void Unit();
    void Collect();
    void Draw();
    IAGPQueryZonePtr GetQueryZoneForName(const std::string& vName, const std::string& vSection = "", const bool& vIsRoot = false);

private:
    void SetQueryZoneForDepth(IAGPQueryZonePtr vQueryZone, GLuint vDepth);
    IAGPQueryZonePtr GetQueryZoneFromDepth(GLuint vDepth);
};

InAppGpuAverageValue::InAppGpuAverageValue() {
    memset(m_PerFrame, 0, sizeof(double) * 60U);
    m_PerFrameIdx = 0;
    m_PerFrameAccum = 0.0;
    m_AverageValue = 0.0;
}

void InAppGpuAverageValue::AddValue(double vValue) {
    if (vValue < m_PerFrame[m_PerFrameIdx]) {
        memset(m_PerFrame, 0, sizeof(double) * 60U);
        m_PerFrameIdx = 0;
        m_PerFrameAccum = 0.0;
        m_AverageValue = 0.0;
    }
    m_PerFrameAccum += vValue - m_PerFrame[m_PerFrameIdx];
    m_PerFrame[m_PerFrameIdx] = vValue;
    m_PerFrameIdx = (m_PerFrameIdx + 1) % 60;
    if (m_PerFrameAccum > 0.0)
        m_AverageValue = m_PerFrameAccum / 60.0;
}

double InAppGpuAverageValue::GetAverage() {
    return m_AverageValue;
}

////////////////////////////////////////////////////////////
/////////////////////// QUERY ZONE /////////////////////////
////////////////////////////////////////////////////////////

GLuint InAppGpuQueryZone::sMaxDepthToOpen = 100U;  // the max by default
bool InAppGpuQueryZone::sShowLeafMode = false;
float InAppGpuQueryZone::sContrastRatio = 4.3f;
bool InAppGpuQueryZone::sActivateLogger = false;

InAppGpuQueryZone::InAppGpuQueryZone(GPU_CONTEXT_PTR vThread, const std::string& vName, const std::string& vSectionName, const bool& vIsRoot)
    : m_ThreadPtr(vThread), puName(vName), m_IsRoot(vIsRoot), m_SectionName(vSectionName) {
    m_StartFrameId = 0;
    m_EndFrameId = 0;
    m_StartTimeStamp = 0;
    m_EndTimeStamp = 0;
    m_ElapsedTime = 0.0;
    puDepth = InAppGpuScopedZone::sCurrentDepth;

    SetCurrentContext(m_ThreadPtr);
    glGenQueries(2, puIds);
}

InAppGpuQueryZone::~InAppGpuQueryZone() {
    SetCurrentContext(m_ThreadPtr);
    glDeleteQueries(2, puIds);

    puName.clear();
    m_StartFrameId = 0;
    m_EndFrameId = 0;
    m_StartTimeStamp = 0;
    m_EndTimeStamp = 0;
    m_ElapsedTime = 0.0;
    m_ThreadPtr = nullptr;
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
        m_AverageStartValue.AddValue((double)(m_StartTimeStamp * 1e-6));  // ns to ms
        m_AverageEndValue.AddValue((double)(m_EndTimeStamp * 1e-6));      // ns to ms
        m_StartTime = m_AverageStartValue.GetAverage();
        m_EndTime = m_AverageEndValue.GetAverage();
        m_ElapsedTime = m_EndTime - m_StartTime;
    }
}

void InAppGpuQueryZone::DrawMetricLabels() {
    if (m_StartFrameId) {
        bool res = false;

        ImGuiTreeNodeFlags flags = 0;
        if (puZonesOrdered.empty())
            flags = ImGuiTreeNodeFlags_Leaf;

        if (m_Highlighted)
            flags |= ImGuiTreeNodeFlags_Framed;

        if (m_IsRoot) {
            res = ImGui::TreeNodeEx(this, flags, "(%u) %s %u : GPU %.2f ms", puDepth, puName.c_str(), m_StartFrameId - 1U, m_ElapsedTime);
        } else {
            res = ImGui::TreeNodeEx(this, flags, "(%u) %s => GPU %.2f ms", puDepth, puName.c_str(), m_ElapsedTime);
        }

        if (ImGui::IsItemHovered())
            m_Highlighted = true;

        if (res) {
            m_Expanded = true;

            ImGui::Indent();

            for (const auto zone : puZonesOrdered) {
                if (zone.use_count()) {
                    zone->DrawMetricLabels();
                }
            }

            ImGui::Unindent();

            ImGui::TreePop();
        } else {
            m_Expanded = false;
        }
    }
}

bool InAppGpuQueryZone::DrawMetricGraph(IAGPQueryZonePtr vParent, uint32_t vDepth) {
    bool m_essed = false;

    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems /* || !m_StartFrameId*/)
        return m_essed;

    const ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const float aw = ImGui::GetContentRegionAvail().x - style.FramePadding.x;

    if (puDepth > InAppGpuQueryZone::sMaxDepthToOpen)
        return m_essed;

    if (!vParent.use_count()) {
        vParent = puThis;
        puTopQuery = puThis;
    }

    if (m_ElapsedTime > 0.0) {
        if (puDepth == 0) {
            m_BarWidth = aw;
            m_BarPos = 0.0f;
        } else if (vParent->m_ElapsedTime > 0.0) {
            puTopQuery = vParent->puTopQuery;
            const float startRatio = (float)((m_StartTime - vParent->m_StartTime) / vParent->m_ElapsedTime);
            const float elapsedRatio = (float)(m_ElapsedTime / vParent->m_ElapsedTime);
            m_BarWidth = vParent->m_BarWidth * elapsedRatio;
            m_BarPos = vParent->m_BarPos + vParent->m_BarWidth * startRatio;
        }

        if ((puZonesOrdered.empty() && InAppGpuQueryZone::sShowLeafMode) || !InAppGpuQueryZone::sShowLeafMode) {
            ImGui::PushID(this);
            m_BarLabel = toStr("%s (%.1f ms | %.1f f/s)", puName.c_str(), m_ElapsedTime, 1000.0f / m_ElapsedTime);
            const char* label = m_BarLabel.c_str();
            const ImGuiID id = window->GetID(label);
            ImGui::PopID();

            const ImVec2 label_size = ImGui::CalcTextSize(label, nullptr, true);
            const float height = label_size.y + style.FramePadding.y * 2.0f;
            const ImVec2 bPos = ImVec2(m_BarPos + style.FramePadding.x, vDepth * height + style.FramePadding.y);
            const ImVec2 bSize = ImVec2(m_BarWidth - style.FramePadding.x, 0.0f);

            const ImVec2 pos = window->DC.CursorPos + bPos;
            const ImVec2 size = ImVec2(m_BarWidth, height);

            const ImRect bb(pos, pos + size);
            bool hovered, held;
            m_essed = ImGui::ButtonBehavior(bb, id, &hovered, &held, ImGuiButtonFlags_PressedOnClick);

            m_Highlighted = false;
            if (hovered) {
                ImGui::SetTooltip("section %s : %s\nElapsed time : %.1f ms\nElapsed FPS : %.1f f/s", m_SectionName.c_str(), puName.c_str(),
                                  m_ElapsedTime, 1000.0f / m_ElapsedTime);
                m_Highlighted = true;  // to highlight label graph by this button
            } else if (m_Highlighted)
                hovered = true;  // highlight this button by the label graph

            // Render
            // const ImU32 col = ImGui::GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
            ImVec4 cv4, hsv = ImVec4((float)(0.5 - m_ElapsedTime * 0.5 / puTopQuery->m_ElapsedTime), 0.5f, 1.0f, 1.0f);
            ImGui::ColorConvertHSVtoRGB(hsv.x, hsv.y, hsv.z, cv4.x, cv4.y, cv4.z);
            cv4.w = 1.0f;
            ImGui::RenderNavHighlight(bb, id);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
            ImGui::RenderFrame(bb.Min, bb.Max, ImGui::ColorConvertFloat4ToU32(cv4), true, 2.0f);
            ImGui::PopStyleVar();
            RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, label, nullptr, &label_size, ImVec2(0.5f, 0.5f), &bb);

            vDepth++;
        }

        // childs
        for (const auto zone : puZonesOrdered) {
            if (zone.use_count()) {
                m_essed |= zone->DrawMetricGraph(puThis, vDepth);
            }
        }
    }

    if (puDepth == 0 && ((puZonesOrdered.empty() && InAppGpuQueryZone::sShowLeafMode) || !InAppGpuQueryZone::sShowLeafMode)) {
        const ImVec2 pos = window->DC.CursorPos;
        const ImVec2 size = ImVec2(aw, ImGui::GetFrameHeight() * (InAppGpuScopedZone::sMaxDepth + 1U));
        ImGui::ItemSize(size);

        const ImRect bb(pos, pos + size);
        const ImGuiID id = window->GetID((puName + "##canvas").c_str());
        if (!ImGui::ItemAdd(bb, id))
            return m_essed;
    }

    return m_essed;
}

////////////////////////////////////////////////////////////
/////////////////////// GL CONTEXT /////////////////////////
////////////////////////////////////////////////////////////

InAppGpuGLContext::InAppGpuGLContext(GPU_CONTEXT_PTR vThread) : m_ThreadPtr(vThread) {
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
                if (ptr.use_count()) {
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
            if (ptr.use_count()) {
                LogError("%*s id not retrieved : %u", ptr->puDepth, "", id);
            }
        }
    }

#ifdef DEBUG_MODE_LOGGING
    DEBUG_MODE_LOGGING("------ End Frame -----");
#endif
}

void InAppGpuGLContext::Draw() {
    if (m_RootZone.use_count()) {
        m_RootZone->DrawMetricGraph();
        if (!InAppGpuQueryZone::sShowLeafMode) {
            m_RootZone->DrawMetricLabels();
        }
    }
}

IAGPQueryZonePtr InAppGpuGLContext::GetQueryZoneForName(const std::string& vName, const std::string& vSection,
                                                                          const bool& vIsRoot) {
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
        if (!m_RootZone.use_count()) {
            res = std::make_shared<InAppGpuQueryZone>(m_ThreadPtr, vName, vSection, vIsRoot);
            if (res.use_count()) {
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
        if (root.use_count()) {
            if (root->puZonesDico.find(vName) == root->puZonesDico.end()) {  // not found
                res = std::make_shared<InAppGpuQueryZone>(m_ThreadPtr, vName, vSection, vIsRoot);
                if (res.use_count()) {
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
            return res;  // happen when profiling is activated inside a m_ofiling zone
        }
    }

    /////////////////////////////////////////////
    //////////////// UTILISATION ////////////////
    /////////////////////////////////////////////

    if (res.use_count()) {
        SetQueryZoneForDepth(res, InAppGpuScopedZone::sCurrentDepth);

        if (res->puName != vName) {
#ifdef _DEBUG
            // at puDepth 0 there is only one frame
            LogDebugError("was registerd at depth %u %s. but we got %s\nwe clear the m_ofiler",  //
                          InAppGpuScopedZone::sCurrentDepth, res->puName.c_str(), vName.c_str());
#endif
            // c'est pas normal, dans le doute on efface le profiler, ca va forcer a le re remplir
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
        if (con.second.use_count()) {
            con.second->Collect();
        }
    }
}

void InAppGpuProfiler::Draw() {
    if (!puIsActive)
        return;

    if (ImGui::BeginMenuBar()) {
        if (InAppGpuScopedZone::sMaxDepth) {
            InAppGpuQueryZone::sMaxDepthToOpen = InAppGpuScopedZone::sMaxDepth;
        }
        ImGui::Text("%s", "Logging");
        ImGui::Checkbox("##logging", &InAppGpuQueryZone::sActivateLogger);

        const char* play_pause_label = "Play";
        if (puIsPaused) {
            play_pause_label = "Pause";
        }
        if (ImGui::Button(play_pause_label)) {
            puIsPaused = !puIsPaused;
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Play/Pause Profiling");
        }

        ImGui::EndMenuBar();
    }

    for (const auto& con : m_Contexts) {
        if (con.second.use_count()) {
            con.second->Draw();
        }
    }
}

IAGPContextPtr InAppGpuProfiler::GetContextPtr(GPU_CONTEXT_PTR vThreadPtr) {
    if (!puIsActive)
        return nullptr;

    if (vThreadPtr != nullptr) {
        if (m_Contexts.find((intptr_t)vThreadPtr) == m_Contexts.end()) {
            m_Contexts[(intptr_t)vThreadPtr] = std::make_shared<InAppGpuGLContext>(vThreadPtr);
        }

        return m_Contexts[(intptr_t)vThreadPtr];
    }

    LogError("GPU_CONTEXT_PTR vThreadPtr is NULL");

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
        auto context_ptr = InAppGpuProfiler::Instance()->GetContextPtr(GetCurrentContext());
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
    if (queryPtr.use_count()) {
        glQueryCounter(queryPtr->puIds[1], GL_TIMESTAMP);
        if (!sCurrentDepth) {
            DEBUG_BREAK;
        }
        sCurrentDepth--;
    }
}

}  // namespace iagp
