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

#pragma once

#include <set>
#include <cmath>
#include <array>
#include <memory>
#include <vector>
#include <string>
#include <imgui.h>
#include <unordered_map>

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif  // IMGUI_DEFINE_MATH_OPERATORS

#ifndef CUSTOM_IN_APP_GPU_PROFILER_CONFIG
#include "InAppGpuProfilerConfig.h"
#else  // CUSTOM_IN_APP_GPU_PROFILER_CONFIG
#include CUSTOM_IN_APP_GPU_PROFILER_CONFIG
#endif  // CUSTOM_IN_APP_GPU_PROFILER_CONFIG

#ifndef IN_APP_GPU_PROFILER_API
#define IN_APP_GPU_PROFILER_API
#endif  // IN_APP_GPU_PROFILER_API

// a main zone for the frame must always been defined for the frame
#define AIGPNewFrame(section, fmt, ...)                                                        \
    auto __IAGP__ScopedMainZone = iagp::InAppGpuScopedZone(true, section, fmt, ##__VA_ARGS__); \
    (void)__IAGP__ScopedMainZone

#define AIGPScoped(section, fmt, ...)                                                          \
    auto __IAGP__ScopedSubZone = iagp::InAppGpuScopedZone(false, section, fmt, ##__VA_ARGS__); \
    (void)__IAGP__ScopedSubZone

#define AIGPCollect iagp::InAppGpuProfiler::Instance()->Collect()

#ifndef RECURSIVE_LEVELS_COUNT
#define RECURSIVE_LEVELS_COUNT 20U
#endif  // RECURSIVE_LEVELS_COUNT

#ifndef MEAN_AVERAGE_LEVELS_COUNT
#define MEAN_AVERAGE_LEVELS_COUNT 60U
#endif  // MEAN_AVERAGE_LEVELS_COUNT

#ifndef GPU_CONTEXT
#define GPU_CONTEXT void*
#endif // GPU_CONTEXT

namespace iagp {

class InAppGpuQueryZone;
typedef std::shared_ptr<InAppGpuQueryZone> IAGPQueryZonePtr;
typedef std::weak_ptr<InAppGpuQueryZone> IAGPQueryZoneWeak;

class InAppGpuGLContext;
typedef std::shared_ptr<InAppGpuGLContext> IAGPContextPtr;
typedef std::weak_ptr<InAppGpuGLContext> IAGPContextWeak;

enum InAppGpuGraphTypeEnum {
    IN_APP_GPU_HORIZONTAL = 0,
    IN_APP_GPU_CIRCULAR,
    IN_APP_GPU_Count
};

template <typename T>
class InAppGpuAverageValue {
private:
    static constexpr uint32_t sCountAverageValues = MEAN_AVERAGE_LEVELS_COUNT;
    T m_PerFrame[sCountAverageValues] = {};
    int m_PerFrameIdx = (T)0;
    T m_PerFrameAccum = (T)0;
    T m_AverageValue = (T)0;

public:
    InAppGpuAverageValue();
    void AddValue(T vValue);
    T GetAverage();
};

template <typename T>
InAppGpuAverageValue<T>::InAppGpuAverageValue() {
    memset(m_PerFrame, 0, sizeof(T) * sCountAverageValues);
    m_PerFrameIdx = (T)0;
    m_PerFrameAccum = (T)0;
    m_AverageValue = (T)0;
}

template <typename T>
void InAppGpuAverageValue<T>::AddValue(T vValue) {
    if (vValue < m_PerFrame[m_PerFrameIdx]) {
        memset(m_PerFrame, 0, sizeof(T) * sCountAverageValues);
        m_PerFrameIdx = (T)0;
        m_PerFrameAccum = (T)0;
        m_AverageValue = (T)0;
    }
    m_PerFrameAccum += vValue - m_PerFrame[m_PerFrameIdx];
    m_PerFrame[m_PerFrameIdx] = vValue;
    m_PerFrameIdx = (m_PerFrameIdx + (T)1) % sCountAverageValues;
    if (m_PerFrameAccum > (T)0) {
        m_AverageValue = m_PerFrameAccum / (T)sCountAverageValues;
    }
}

template <typename T>
T InAppGpuAverageValue<T>::GetAverage() {
    return m_AverageValue;
}

class IN_APP_GPU_PROFILER_API InAppGpuQueryZone {
public:
    struct circularSettings {
        float count_point = 20.0f;  // 60 for 2pi
        float scaleX = 1.0f;
        float scaleY = 1.0f;
        float base_radius = 50.0f;
        float space = 5.0f;
        float thick = 10.0f;
    };

public:
    static GLuint sMaxDepthToOpen;
    static bool sShowLeafMode;
    static float sContrastRatio;
    static bool sActivateLogger;
    static std::vector<IAGPQueryZoneWeak> sTabbedQueryZones;
    static IAGPQueryZonePtr create(GPU_CONTEXT vContext, const std::string& vName, const std::string& vSectionName, const bool& vIsRoot = false);
    static circularSettings sCircularSettings;

public:
    GLuint depth = 0U;  // the depth of the QueryZone
    GLuint ids[2] = {0U, 0U};
    std::vector<IAGPQueryZonePtr> zonesOrdered;
    std::unordered_map<std::string, IAGPQueryZonePtr> zonesDico;  // main container
    std::string name;
    std::string imGuiLabel;
    std::string imGuiTitle;
    IAGPQueryZonePtr parentPtr = nullptr;
    IAGPQueryZonePtr rootPtr = nullptr;

private:
    IAGPQueryZoneWeak m_This;
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
    InAppGpuAverageValue<GLuint64> m_AverageStartValue;
    InAppGpuAverageValue<GLuint64> m_AverageEndValue;
    GPU_CONTEXT m_Context;
    std::string m_BarLabel;
    std::string m_SectionName;
    ImVec4 cv4;
    ImVec4 hsv;
    InAppGpuGraphTypeEnum m_GraphType = InAppGpuGraphTypeEnum::IN_APP_GPU_HORIZONTAL;

    // fil d'ariane
    std::array<IAGPQueryZoneWeak, RECURSIVE_LEVELS_COUNT> m_BreadCrumbTrail; // the parent cound is done by current depth

    // circular
    const float _1PI_ = 3.141592653589793238462643383279f;
    const float _2PI_ = 6.283185307179586476925286766559f;
    const ImU32 m_BlackU32 = ImGui::GetColorU32(ImVec4(0, 0, 0, 1));
    ImVec2 m_P0, m_P1, m_LP0, m_LP1;

public:
    InAppGpuQueryZone() = default;
    InAppGpuQueryZone(GPU_CONTEXT vContext, const std::string& vName, const std::string& vSectionName, const bool& vIsRoot = false);
    ~InAppGpuQueryZone();
    void Clear();
    void SetStartTimeStamp(const GLuint64& vValue);
    void SetEndTimeStamp(const GLuint64& vValue);
    void ComputeElapsedTime();
    void DrawDetails();
    bool DrawFlamGraph(InAppGpuGraphTypeEnum vGraphType, IAGPQueryZoneWeak& vOutSelectedQuery, IAGPQueryZoneWeak vParent = {},
                       uint32_t vDepth = 0);
    void updateBreadCrumbTrail();
    void DrawBreadCrumbTrail(IAGPQueryZoneWeak& vOutSelectedQuery);

private:
    bool m_ComputeRatios(IAGPQueryZonePtr vRoot, IAGPQueryZoneWeak vParent, uint32_t vDepth, float& vOutStartRatio, float& vOutSizeRatio);
    bool m_DrawHorizontalFlameGraph(IAGPQueryZonePtr vRoot, IAGPQueryZoneWeak& vOutSelectedQuery, IAGPQueryZoneWeak vParent,
                                    uint32_t vDepth);
    bool m_DrawCircularFlameGraph(IAGPQueryZonePtr vRoot, IAGPQueryZoneWeak& vOutSelectedQuery, IAGPQueryZoneWeak vParent,
                                  uint32_t vDepth);
};

class IN_APP_GPU_PROFILER_API InAppGpuGLContext {
private:
    IAGPContextWeak m_This;
    GPU_CONTEXT m_Context;
    IAGPQueryZonePtr m_RootZone = nullptr;
    IAGPQueryZoneWeak m_SelectedQuery; // query to show the flamegraph in this context
    std::unordered_map<GLuint, IAGPQueryZonePtr> m_QueryIDToZone;    // Get the zone for a query id because a query have to id's : start and end
    std::unordered_map<GLuint, IAGPQueryZonePtr> m_DepthToLastZone;  // last zone registered at this depth
    std::set<GLuint> m_PendingUpdate;                                // some queries msut but retrieveds

public:
    static IAGPContextPtr create(GPU_CONTEXT vContext);

public:
    InAppGpuGLContext(GPU_CONTEXT vContext);
    void Clear();
    void Init();
    void Unit();
    void Collect();
    void DrawFlamGraph(const InAppGpuGraphTypeEnum& vGraphType);
    void DrawDetails();
    IAGPQueryZonePtr GetQueryZoneForName(const std::string& vName, const std::string& vSection = "", const bool& vIsRoot = false);

private:
    void m_SetQueryZoneForDepth(IAGPQueryZonePtr vQueryZone, GLuint vDepth);
    IAGPQueryZonePtr m_GetQueryZoneFromDepth(GLuint vDepth);
};

class IN_APP_GPU_PROFILER_API InAppGpuScopedZone {
public:
    static GLuint sCurrentDepth;  // current depth catched by Profiler
    static GLuint sMaxDepth;      // max depth catched ever

public:
    IAGPQueryZonePtr queryPtr = nullptr;

public:
    InAppGpuScopedZone(const bool& vIsRoot, const std::string& vSection, const char* fmt, ...);
    ~InAppGpuScopedZone();
};

class IN_APP_GPU_PROFILER_API InAppGpuProfiler {
public:
    typedef std::function<bool(const char*, bool*, ImGuiWindowFlags)> ImGuiBeginFunctor;
    typedef std::function<void()> ImGuiEndFunctor;

public:
    static bool sIsActive;
    static bool sIsPaused;

private:
    std::unordered_map<intptr_t, IAGPContextPtr> m_Contexts;
    InAppGpuGraphTypeEnum m_GraphType = InAppGpuGraphTypeEnum::IN_APP_GPU_HORIZONTAL;
    IAGPQueryZoneWeak m_SelectedQuery;
    int32_t m_QueryZoneToClose = -1;
    ImGuiBeginFunctor m_ImGuiBeginFunctor =                                     //
        [](const char* vLabel, bool* pOpen, ImGuiWindowFlags vFlags) -> bool {  //
        return ImGui::Begin(vLabel, pOpen, vFlags);
    };
    ImGuiEndFunctor m_ImGuiEndFunctor = []() { ImGui::End(); };
    bool m_ShowDetails = false;

public:
    void Clear();
    void Collect();
    void DrawFlamGraph(const char* vLabel, bool* pOpen, ImGuiWindowFlags vFlags = 0);
    void DrawFlamGraphNoWin();
    void DrawFlamGraphChilds(ImGuiWindowFlags vFlags = 0);
    void SetImGuiBeginFunctor(const ImGuiBeginFunctor& vImGuiBeginFunctor);
    void SetImGuiEndFunctor(const ImGuiEndFunctor& vImGuiEndFunctor);
    void DrawDetails(ImGuiWindowFlags vFlags = 0);
    void DrawDetailsNoWin();
    IAGPContextPtr GetContextPtr(GPU_CONTEXT vContext);
    InAppGpuGraphTypeEnum& GetGraphTypeRef() {
        return m_GraphType;
    }

private:
    void m_DrawMenuBar();

public:
    static InAppGpuProfiler* Instance() {
        static InAppGpuProfiler _instance;
        return &_instance;
    }

protected:
    InAppGpuProfiler();
    InAppGpuProfiler(const InAppGpuProfiler&);
    InAppGpuProfiler& operator=(const InAppGpuProfiler&);
    ~InAppGpuProfiler();
};

}  // namespace iagp
