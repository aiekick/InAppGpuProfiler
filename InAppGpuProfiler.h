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
#include <cstdint>
#include <vector>
#include <string>
#include <functional>
#include <unordered_map>

// Clang/GCC warnings with -Weverything
#if defined(__clang__)
#if __has_warning("-Wunknown-warning-option")
#pragma clang diagnostic ignored \
    "-Wunknown-warning-option"  // warning: unknown warning group 'xxx'                      // not all warnings are known by all Clang versions and
                                // they tend to be rename-happy.. so ignoring warnings triggers new warnings on some configuration. Great!
#endif
#pragma clang diagnostic ignored "-Wunknown-pragmas"  // warning: unknown warning group 'xxx'
#pragma clang diagnostic ignored "-Wold-style-cast"   // warning: use of old-style cast                            // yes, they are more terse.
#pragma clang diagnostic ignored "-Wfloat-equal"  // warning: comparing floating point with == or != is unsafe // storing and comparing against same
                                                  // constants (typically 0.0f) is ok.
#pragma clang diagnostic ignored "-Wformat-nonliteral"      // warning: format string is not a string literal            // passing non-literal to
                                                            // vsnformat(). yes, user passing incorrect format strings can crash the code.
#pragma clang diagnostic ignored "-Wexit-time-destructors"  // warning: declaration requires an exit-time destructor     // exit-time destruction
                                                            // order is undefined. if MemFree() leads to users code that has been disabled before exit
                                                            // it might cause problems. ImGui coding style welcomes static/globals.
#pragma clang diagnostic ignored "-Wglobal-constructors"  // warning: declaration requires a global destructor         // similar to above, not sure
                                                          // what the exact difference is.
#pragma clang diagnostic ignored "-Wsign-conversion"      // warning: implicit conversion changes signedness
#pragma clang diagnostic ignored "-Wformat-pedantic"  // warning: format specifies type 'void *' but the argument has type 'xxxx *' // unreasonable,
                                                      // would lead to casting every %p arg to void*. probably enabled by -pedantic.
#pragma clang diagnostic ignored "-Wint-to-void-pointer-cast"       // warning: cast to 'void *' from smaller integer type 'int'
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"  // warning: zero as null pointer constant                    // some standard
                                                                    // header variations use #define NULL 0
#pragma clang diagnostic ignored "-Wdouble-promotion"  // warning: implicit conversion from 'float' to 'double' when passing argument to function  //
                                                       // using printf() is a misery with this as C++ va_arg ellipsis changes float to double.
#pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"  // warning: implicit conversion from 'xxx' to 'float' may lose precision
#elif defined(__GNUC__)
// We disable -Wpragmas because GCC doesn't provide a has_warning equivalent and some forks/patches may not follow the warning/version association.
#pragma GCC diagnostic ignored "-Wpragmas"              // warning: unknown option after '#pragma GCC diagnostic' kind
#pragma GCC diagnostic ignored "-Wunused-function"      // warning: 'xxxx' defined but not used
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"  // warning: cast to pointer from integer of different size
#pragma GCC diagnostic ignored "-Wformat"             // warning: format '%p' expects argument of type 'void*', but argument 6 has type 'ImGuiWindow*'
#pragma GCC diagnostic ignored "-Wdouble-promotion"   // warning: implicit conversion from 'float' to 'double' when passing argument to function
#pragma GCC diagnostic ignored "-Wconversion"         // warning: conversion to 'xxxx' from 'xxxx' may alter its value
#pragma GCC diagnostic ignored "-Wformat-nonliteral"  // warning: format not a string literal, format string not checked
#pragma GCC diagnostic ignored "-Wstrict-overflow"  // warning: assuming signed overflow does not occur when assuming that (X - c) > X is always false
#pragma GCC diagnostic ignored "-Wclass-memaccess"  // [__GNUC__ >= 8] warning: 'memset/memcpy' clearing/writing an object of type 'xxxx' with no
                                                    // trivial copy-assignment; use assignment or value-initialization instead
#endif

#if defined(__WIN32__) || defined(WIN32) || defined(_WIN32) || defined(__WIN64__) || defined(WIN64) || defined(_WIN64) || defined(_MSC_VER)
#if defined(InAppGpuProfiler_EXPORTS)
#define IN_APP_GPU_PROFILER_API __declspec(dllexport)
#elif defined(BUILD_IAGP_SHARED_LIBS)
#define IN_APP_GPU_PROFILER_API __declspec(dllimport)
#else
#define IN_APP_GPU_PROFILER_API
#endif
#else
#define IN_APP_GPU_PROFILER_API
#endif

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif  // IMGUI_DEFINE_MATH_OPERATORS

#ifndef CUSTOM_IN_APP_GPU_PROFILER_CONFIG
#include "InAppGpuProfilerConfig.h"
#else // CUSTOM_IN_APP_GPU_PROFILER_CONFIG
#include CUSTOM_IN_APP_GPU_PROFILER_CONFIG
#endif // CUSTOM_IN_APP_GPU_PROFILER_CONFIG

#ifndef IN_APP_GPU_PROFILER_API
#define IN_APP_GPU_PROFILER_API
#endif // IN_APP_GPU_PROFILER_API

#ifdef VULKAN_PROFILING
// a main zone for the frame must always been defined for the frame
#define AIGPNewFrame(commandBuffer, contextPtr, section, fmt, ...)                                                                   \
    auto __IAGP__ScopedMainZone = iagp::InAppGpuScopedZone(commandBuffer, true, contextPtr, nullptr, section, fmt, ##__VA_ARGS__); \
    (void)__IAGP__ScopedMainZone

#define AIGPScoped(commandBuffer, contextPtr, section, fmt, ...)                                                                     \
    auto __IAGP__ScopedSubZone = iagp::InAppGpuScopedZone(commandBuffer, false, contextPtr, nullptr, section, fmt, ##__VA_ARGS__); \
    (void)__IAGP__ScopedSubZone

#define AIGPScopedPtr(commandBuffer, contextPtr, ptr, section, fmt, ...)                                                         \
    auto __IAGP__ScopedSubZone = iagp::InAppGpuScopedZone(commandBuffer, false, contextPtr, ptr, section, fmt, ##__VA_ARGS__); \
    (void)__IAGP__ScopedSubZone

#define AIGPCollect(resetQueryPoolCommandBuffer) iagp::InAppGpuProfiler::Instance()->Collect(resetQueryPoolCommandBuffer)
#endif // VULKAN_PROFILING

#ifdef OPENGl_PROFILING
// a main zone for the frame must always been defined for the frame
#define AIGPNewFrame(section, fmt, ...)                                                                   \
    auto __IAGP__ScopedMainZone = iagp::InAppGpuScopedZone(true, nullptr, section, fmt, ##__VA_ARGS__); \
    (void)__IAGP__ScopedMainZone

#define AIGPScoped(section, fmt, ...)                                                                     \
    auto __IAGP__ScopedSubZone = iagp::InAppGpuScopedZone(false, nullptr, section, fmt, ##__VA_ARGS__); \
    (void)__IAGP__ScopedSubZone

#define AIGPScopedPtr(ptr, section, fmt, ...)                                                         \
    auto __IAGP__ScopedSubZone = iagp::InAppGpuScopedZone(false, ptr, section, fmt, ##__VA_ARGS__); \
    (void)__IAGP__ScopedSubZone

#define AIGPCollect iagp::InAppGpuProfiler::Instance()->Collect()
#endif // OPENGl_PROFILING

#ifndef IAGP_RECURSIVE_LEVELS_COUNT
#define IAGP_RECURSIVE_LEVELS_COUNT 20U
#endif  // RECURSIVE_LEVELS_COUNT

#ifndef IAGP_MEAN_AVERAGE_LEVELS_COUNT
#define IAGP_MEAN_AVERAGE_LEVELS_COUNT 60U
#endif  // MEAN_AVERAGE_LEVELS_COUNT

#ifndef IAGP_GPU_CONTEXT
#define IAGP_GPU_CONTEXT void*
#endif  // GPU_CONTEXT

namespace iagp {
#ifdef VULKAN_PROFILING
typedef uint64_t vkTimeStamp;
#endif

class InAppGpuQueryZone;
typedef std::shared_ptr<InAppGpuQueryZone> IAGPQueryZonePtr;
typedef std::weak_ptr<InAppGpuQueryZone> IAGPQueryZoneWeak;

class InAppGpuContext;
typedef std::shared_ptr<InAppGpuContext> IAGPContextPtr;
typedef std::weak_ptr<InAppGpuContext> IAGPContextWeak;

enum InAppGpuGraphTypeEnum { IN_APP_GPU_HORIZONTAL = 0, IN_APP_GPU_CIRCULAR, IN_APP_GPU_Count };

template <typename T>
class InAppGpuAverageValue {
private:
    static constexpr uint32_t sCountAverageValues = IAGP_MEAN_AVERAGE_LEVELS_COUNT;
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
    static uint32_t sMaxDepthToOpen;
    static bool sShowLeafMode;
    static float sContrastRatio;
    static bool sActivateLogger;
    static std::vector<IAGPQueryZoneWeak> sTabbedQueryZones;
    static IAGPQueryZonePtr create(
        IAGP_GPU_CONTEXT vContext, const void* vPtr, const std::string& vName, const std::string& vSectionName, const bool& vIsRoot = false);
    static circularSettings sCircularSettings;

public:
    uint32_t depth = 0U;  // the depth of the QueryZone
    uint32_t ids[2] = {0U, 0U};
    std::vector<IAGPQueryZonePtr> zonesOrdered;
    std::unordered_map<const void*, std::unordered_map<std::string, IAGPQueryZonePtr>> zonesDico;  // main container
    std::string name;
    std::string imGuiLabel;
    std::string imGuiTitle;
    IAGPQueryZonePtr parentPtr = nullptr;
    IAGPQueryZonePtr rootPtr = nullptr;
    uint32_t current_count = 0U;
    uint32_t last_count = 0U;
#ifdef VULKAN_PROFILING
    VkCommandBuffer commandBuffer;
    VkQueryPool queryPool;
#endif

private:
    IAGPQueryZoneWeak m_This;
    bool m_IsRoot = false;
    const void* m_Ptr = nullptr;
    double m_ElapsedTime = 0.0;
    double m_StartTime = 0.0;
    double m_EndTime = 0.0;
    uint32_t m_StartFrameId = 0;
    uint32_t m_EndFrameId = 0;
    uint64_t m_StartTimeStamp = 0;
    uint64_t m_EndTimeStamp = 0;
    bool m_Expanded = false;
    bool m_Highlighted = false;
    InAppGpuAverageValue<uint64_t> m_AverageStartValue;
    InAppGpuAverageValue<uint64_t> m_AverageEndValue;
    IAGP_GPU_CONTEXT m_Context;
    std::string m_BarLabel;
    std::string m_SectionName;
    ImVec4 cv4;
    ImVec4 hsv;
    InAppGpuGraphTypeEnum m_GraphType = InAppGpuGraphTypeEnum::IN_APP_GPU_HORIZONTAL;

    // fil d'ariane
    std::array<IAGPQueryZoneWeak, IAGP_RECURSIVE_LEVELS_COUNT> m_BreadCrumbTrail;  // the parent cound is done by current depth

    // circular
    const float _1PI_ = 3.141592653589793238462643383279f;
    const float _2PI_ = 6.283185307179586476925286766559f;
    const ImU32 m_BlackU32 = ImGui::GetColorU32(ImVec4(0, 0, 0, 1));
    ImVec2 m_P0, m_P1, m_LP0, m_LP1;

public:
    InAppGpuQueryZone() = default;
    InAppGpuQueryZone(
        IAGP_GPU_CONTEXT vContext, const void* vPtr, const std::string& vName, const std::string& vSectionName, const bool& vIsRoot = false);
    ~InAppGpuQueryZone();
    void Clear();
    void SetStartTimeStamp(const uint64_t& vValue);
    void SetEndTimeStamp(const uint64_t& vValue);
    void ComputeElapsedTime();
    void DrawDetails();
    bool DrawFlamGraph(InAppGpuGraphTypeEnum vGraphType,  //
        IAGPQueryZoneWeak& vOutSelectedQuery,               //
        IAGPQueryZoneWeak vParent = {},                     //
        uint32_t vDepth = 0);
    void UpdateBreadCrumbTrail();
    void DrawBreadCrumbTrail(IAGPQueryZoneWeak& vOutSelectedQuery);

private:
    void m_DrawList_DrawBar(const char* vLabel, const ImRect& vRect, const ImVec4& vColor, const bool& vHovered);
    bool m_ComputeRatios(IAGPQueryZonePtr vRoot, IAGPQueryZoneWeak vParent, uint32_t vDepth, float& vOutStartRatio, float& vOutSizeRatio);
    bool m_DrawHorizontalFlameGraph(IAGPQueryZonePtr vRoot, IAGPQueryZoneWeak& vOutSelectedQuery, IAGPQueryZoneWeak vParent, uint32_t vDepth);
    bool m_DrawCircularFlameGraph(IAGPQueryZonePtr vRoot, IAGPQueryZoneWeak& vOutSelectedQuery, IAGPQueryZoneWeak vParent, uint32_t vDepth);
};

class IN_APP_GPU_PROFILER_API InAppGpuContext {
private:
    IAGPContextWeak m_This;
    IAGP_GPU_CONTEXT m_Context;
    IAGPQueryZonePtr m_RootZone = nullptr;
    IAGPQueryZoneWeak m_SelectedQuery;                                 // query to show the flamegraph in this context
    std::unordered_map<uint32_t, IAGPQueryZonePtr> m_QueryIDToZone;    // Get the zone for a query id because a query have to id's : start and end
    std::unordered_map<uint32_t, IAGPQueryZonePtr> m_DepthToLastZone;  // last zone registered at this depth
#ifdef OPENGl_PROFILING
    std::set<uint32_t> m_PendingUpdate;                                // some queries msut but retrieveds
#endif

#ifdef VULKAN_PROFILING
    VkPhysicalDevice m_PhysicalDevice = {};
    VkDevice m_LogicalDevice = {};
    float m_TimeStampPeriod = 0.0f;
    std::vector<vkTimeStamp> m_TimeStampMeasures;
    VkQueryPool m_QueryPool = {};
    size_t m_QueryTail = 0U;
    size_t m_QueryHead = 0U;
    size_t m_QueryCount = 0U;
#endif

public:
    static IAGPContextPtr create(IAGP_GPU_CONTEXT vContext);

public:
    InAppGpuContext(IAGP_GPU_CONTEXT vContext);
    void Clear();
    void Unit();
#ifdef VULKAN_PROFILING
    bool Init(VkPhysicalDevice vPhysicalDevice, VkDevice vLogicalDevice, const uint32_t& vMaxQueryCount);
    int32_t GetNextQueryId();
    VkQueryPool GetQueryPool();
#endif
    void Collect(
#ifdef VULKAN_PROFILING
        VkCommandBuffer vResetQueryPoolCmd
#endif
    );
    void DrawFlamGraph(const InAppGpuGraphTypeEnum& vGraphType);
    void DrawDetails();
    IAGPQueryZonePtr GetQueryZoneForName(const void* vPtr, const std::string& vName, const std::string& vSection = "", const bool& vIsRoot = false);

    bool BeginMarkTime(
#ifdef VULKAN_PROFILING
        const VkCommandBuffer& vCmd,
#endif
        InAppGpuQueryZone* vQueryPtr
    );
    void EndMarkTime(
#ifdef VULKAN_PROFILING
    const VkCommandBuffer& vCmd,
#endif
        InAppGpuQueryZone* vQueryPtr
    );

private:
    void m_SetQueryZoneForDepth(IAGPQueryZonePtr vQueryZone, uint32_t vDepth);
    IAGPQueryZonePtr m_GetQueryZoneFromDepth(uint32_t vDepth);
};

class IN_APP_GPU_PROFILER_API InAppGpuScopedZone {
public:
    static uint32_t sCurrentDepth;  // current depth catched by Profiler
    static uint32_t sMaxDepth;      // max depth catched ever

public:
    IAGPContextPtr contextPtr = nullptr;
    IAGPQueryZonePtr queryPtr = nullptr;
    VkCommandBuffer commandBuffer = nullptr;

public:
    InAppGpuScopedZone(             //
#ifdef VULKAN_PROFILING
        const VkCommandBuffer& vCmd,  //
#endif
        const bool& vIsRoot,          //
#ifdef VULKAN_PROFILING
        void* vContextPtr,            //
#endif
        const void* vPtr,             //
        const std::string& vSection,  //
        const char* fmt,              //
        ...);                         //
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
    bool m_IsLoaded = false;

public:
#ifdef VULKAN_PROFILING
    bool addContext(void* vContextPtr, VkPhysicalDevice vPhysicalDevice, VkDevice vLogicalDevice, const uint32_t& vMaxQueryCount);
#endif
    void Destroy();
    void Collect(
#ifdef VULKAN_PROFILING
        VkCommandBuffer vResetQueryPoolCmd
#endif
    );
    IAGPContextPtr GetContextPtr(IAGP_GPU_CONTEXT vContext);
    InAppGpuGraphTypeEnum& GetGraphTypeRef() {
        return m_GraphType;
    }

public: // imgui
    void DrawFlamGraph(const char* vLabel, bool* pOpen, ImGuiWindowFlags vFlags = 0);
    void DrawFlamGraphNoWin();
    void DrawFlamGraphChilds(ImGuiWindowFlags vFlags = 0);
    void SetImGuiBeginFunctor(const ImGuiBeginFunctor& vImGuiBeginFunctor);
    void SetImGuiEndFunctor(const ImGuiEndFunctor& vImGuiEndFunctor);
    void DrawDetails(ImGuiWindowFlags vFlags = 0);
    void DrawDetailsNoWin();

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
