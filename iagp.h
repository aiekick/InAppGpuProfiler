/*
Copyright 2022-2023 Stephane Cuillerdier (aka aiekick)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#pragma once

#include <set>
#include <cmath>
#include <array>
#include <stack>
#include <memory>
#include <cstdint>
#include <vector>
#include <string>
#include <functional>
#include <unordered_map>


#if defined(__WIN32__) || defined(WIN32) || defined(_WIN32) || defined(__WIN64__) || defined(WIN64) || defined(_WIN64) || defined(_MSC_VER)
#if defined(ImAppGpuProfiler_EXPORTS)
#define IAGP_API __declspec(dllexport)
#elif defined(BUILD_IN_APP_GPU_PROFILER_SHARED_LIBS)
#define IAGP_API __declspec(dllimport)
#else
#define IAGP_API
#endif
#else
#define IAGP_API
#endif

#ifdef IMGUI_INCLUDE
#include IMGUI_INCLUDE
#else  // IMGUI_INCLUDE
#include <imgui.h>
#endif  // IMGUI_INCLUDE

#ifndef CUSTOM_IN_APP_GPU_PROFILER_CONFIG
#include "iagpConfig.h"
#else  // CUSTOM_IN_APP_GPU_PROFILER_CONFIG
#include CUSTOM_IN_APP_GPU_PROFILER_CONFIG
#endif  // CUSTOM_IN_APP_GPU_PROFILER_CONFIG

#include <set>
#include <cmath>
#include <array>
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <unordered_map>

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif  // IMGUI_DEFINE_MATH_OPERATORS

///////////////////////////////////
//// MACROS ///////////////////////
///////////////////////////////////

/*
#define iagpBeginFrame(label) GaiApi::Profiler::Instance()->BeginFrame(label)
#define iagpEndFrame GaiApi::Profiler::Instance()->EndFrame()
#define iagpCollectFrame GaiApi::Profiler::Instance()->Collect()

#define iagpBeginZone(commandBuffer, section, fmt, ...) \
    GaiApi::Profiler::Instance()->beginChildZone(commandBuffer, nullptr, section, fmt, ##__VA_ARGS__)
#define iagpBeginZonePtr(commandBuffer, ptr, section, fmt, ...) \
    GaiApi::Profiler::Instance()->beginChildZone(commandBuffer, ptr, section, fmt, ##__VA_ARGS__);
#define iagpEndZone(commandBuffer) GaiApi::Profiler::Instance()->endChildZone(commandBuffer)

#define iagpScopedStages(stages, commandBuffer, section, fmt, ...)                                                         \
    auto __iagp__ScopedChildZone = GaiApi::vkScopedChildZone(stages, commandBuffer, nullptr, section, fmt, ##__VA_ARGS__); \
    (void)__iagp__ScopedChildZone
#define iagpScopedStagesPtr(stages, commandBuffer, ptr, section, fmt, ...)                                             \
    auto __iagp__ScopedChildZone = GaiApi::vkScopedChildZone(stages, commandBuffer, ptr, section, fmt, ##__VA_ARGS__); \
    (void)__iagp__ScopedChildZone

#define iagpScoped(commandBuffer, section, fmt, ...)                                                               \
    auto __iagp__ScopedChildZone = GaiApi::vkScopedChildZone(commandBuffer, nullptr, section, fmt, ##__VA_ARGS__); \
    (void)__iagp__ScopedChildZone
#define iagpScopedPtr(commandBuffer, ptr, section, fmt, ...)                                                   \
    auto __iagp__ScopedChildZone = GaiApi::vkScopedChildZone(commandBuffer, ptr, section, fmt, ##__VA_ARGS__); \
    (void)__iagp__ScopedChildZone

#define iagpScopedStagesNoCmd(stages, section, fmt, ...)                                                              \
    auto __iagp__ScopedChildZoneNoCmd = GaiApi::vkScopedChildZoneNoCmd(stages, nullptr, section, fmt, ##__VA_ARGS__); \
    (void)__iagp__ScopedChildZoneNoCmd
#define iagpScopedStagesPtrNoCmd(stages, ptr, section, fmt, ...)                                                  \
    auto __iagp__ScopedChildZoneNoCmd = GaiApi::vkScopedChildZoneNoCmd(stages, ptr, section, fmt, ##__VA_ARGS__); \
    (void)__iagp__ScopedChildZoneNoCmd

#define iagpScopedNoCmd(section, fmt, ...)                                                                    \
    auto __iagp__ScopedChildZoneNoCmd = GaiApi::vkScopedChildZoneNoCmd(nullptr, section, fmt, ##__VA_ARGS__); \
    (void)__iagp__ScopedChildZoneNoCmd
#define iagpScopedPtrNoCmd(ptr, section, fmt, ...)                                                        \
    auto __iagp__ScopedChildZoneNoCmd = GaiApi::vkScopedChildZoneNoCmd(ptr, section, fmt, ##__VA_ARGS__); \
    (void)__iagp__ScopedChildZoneNoCmd

#define iagpBeginZoneNoCmd(section, fmt, ...) GaiApi::Profiler::Instance()->beginChildZoneNoCmd(nullptr, section, fmt, ##__VA_ARGS__)
#define iagpBeginZonePtrNoCmd(ptr, section, fmt, ...) GaiApi::Profiler::Instance()->beginChildZoneNoCmd(ptr, section, fmt, ##__VA_ARGS__);
#define iagpEndZoneNoCmd GaiApi::Profiler::Instance()->endChildZoneNoCmd()
*/

///////////////////////////////////
///////////////////////////////////
///////////////////////////////////

#ifndef iagp_RECURSIVE_LEVELS_COUNT
#define iagp_RECURSIVE_LEVELS_COUNT 20U
#endif  // RECURSIVE_LEVELS_COUNT

#ifndef iagp_MEAN_AVERAGE_LEVELS_COUNT
#define iagp_MEAN_AVERAGE_LEVELS_COUNT 60U
#endif  // MEAN_AVERAGE_LEVELS_COUNT

namespace iagp {

typedef uint64_t TimeStamp;

class iagpQueryZone;
typedef std::shared_ptr<iagpQueryZone> iagpQueryZonePtr;
typedef std::weak_ptr<iagpQueryZone> iagpQueryZoneWeak;

class Profiler;
typedef std::shared_ptr<Profiler> ProfilerPtr;
typedef std::weak_ptr<Profiler> ProfilerWeak;

enum class iagpGraphTypeEnum {  //
    HORIZONTAL = 0,
    CIRCULAR,
    Count
};

template <typename T>
class iagpAverageValue {
private:
    static constexpr uint32_t sCountAverageValues = iagp_MEAN_AVERAGE_LEVELS_COUNT;
    T m_PerFrame[sCountAverageValues] = {};
    int m_PerFrameIdx = (T)0;
    T m_PerFrameAccum = (T)0;
    T m_AverageValue = (T)0;

public:
    iagpAverageValue();
    void AddValue(T vValue);
    T GetAverage();
};

template <typename T>
iagpAverageValue<T>::iagpAverageValue() {
    memset(m_PerFrame, 0, sizeof(T) * sCountAverageValues);
    m_PerFrameIdx = (T)0;
    m_PerFrameAccum = (T)0;
    m_AverageValue = (T)0;
}

template <typename T>
void iagpAverageValue<T>::AddValue(T vValue) {
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
T iagpAverageValue<T>::GetAverage() {
    return m_AverageValue;
}

class IAGP_API iagpQueryZone {
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
    static uint32_t sCurrentDepth;  // current depth catched by Profiler
    static uint32_t sMaxDepth;      // max depth catched ever
    static std::vector<iagpQueryZoneWeak> sTabbedQueryZones;
    static iagpQueryZonePtr create(
        void* vThreadPtr, const void* vPtr, const std::string& vName, const std::string& vSectionName, const bool& vIsRoot = false);
    static circularSettings sCircularSettings;

public:
    uint32_t depth = 0U;  // the depth of the QueryZone
    // inc the query each calls (for identify where a id is called
    // many time per frame but not reseted before and causse layer issue)
    uint32_t calledCountPerFrame = 0U;
    std::vector<iagpQueryZonePtr> zonesOrdered;
    std::unordered_map<const void*, std::unordered_map<std::string, iagpQueryZonePtr>> zonesDico;  // main container
    std::string name;
    std::string imGuiLabel;
    std::string imGuiTitle;
    iagpQueryZonePtr parentPtr = nullptr;
    iagpQueryZonePtr rootPtr = nullptr;
    uint32_t current_count = 0U;
    uint32_t last_count = 0U;
#ifdef IAGP_VULKAN
    VkCommandBuffer commandBuffer;
    VkQueryPool queryPool;
#else
#endif

private:
    uint32_t ids[2] = {0U, 0U};
    iagpQueryZoneWeak m_This;
    bool m_IsRoot = false;
    //const void* m_Ptr = nullptr;
    double m_ElapsedTime = 0.0;
    double m_StartTime = 0.0;
    double m_EndTime = 0.0;
    uint32_t m_StartFrameId = 0;
    uint32_t m_EndFrameId = 0;
    uint64_t m_StartTimeStamp = 0;
    uint64_t m_EndTimeStamp = 0;
    bool m_Expanded = false;
    bool m_Highlighted = false;
    iagpAverageValue<uint64_t> m_AverageStartValue;
    iagpAverageValue<uint64_t> m_AverageEndValue;
    //void* m_ThreadPtr = nullptr;
    std::string m_BarLabel;
    std::string m_SectionName;
    ImVec4 cv4;
    ImVec4 hsv;
   // iagpGraphTypeEnum m_GraphType = iagpGraphTypeEnum::IN_APP_GPU_HORIZONTAL;

    // fil d'ariane
    std::array<iagpQueryZoneWeak, iagp_RECURSIVE_LEVELS_COUNT> m_BreadCrumbTrail;  // the parent cound is done by current depth

    // circular
    const float _1PI_ = 3.141592653589793238462643383279f;
    //const float _2PI_ = 6.283185307179586476925286766559f;
    const ImU32 m_BlackU32 = ImGui::GetColorU32(ImVec4(0, 0, 0, 1));
    ImVec2 m_P0, m_P1, m_LP0, m_LP1;

public:
    iagpQueryZone() = default;
    iagpQueryZone(void* vThreadPtr, const void* vPtr, const std::string& vName, const std::string& vSectionName, const bool& vIsRoot = false);
    ~iagpQueryZone();
    void Clear();
    const uint32_t& GetIdForWrite(const size_t& vIdx);
    const uint32_t& GetId(const size_t& vIdx) const;
    void SetId(const size_t& vIdx, const uint32_t& vID);
    bool wasSeen() const;
    void NewFrame();
    void SetStartTimeStamp(const uint64_t& vValue);
    void SetEndTimeStamp(const uint64_t& vValue);
    void ComputeElapsedTime();
    void DrawDetails();
    bool DrawFlamGraph(iagpGraphTypeEnum vGraphType,  //
        iagpQueryZoneWeak& vOutSelectedQuery,         //
        iagpQueryZoneWeak vParent = {},               //
        uint32_t vDepth = 0);
    void UpdateBreadCrumbTrail();
    void DrawBreadCrumbTrail(iagpQueryZoneWeak& vOutSelectedQuery);

private:
    void m_DrawList_DrawBar(const char* vLabel, const ImRect& vRect, const ImVec4& vColor, const bool& vHovered);
    bool m_ComputeRatios(iagpQueryZonePtr vRoot, iagpQueryZoneWeak vParent, uint32_t vDepth, float& vOutStartRatio, float& vOutSizeRatio);
    bool m_DrawHorizontalFlameGraph(iagpQueryZonePtr vRoot, iagpQueryZoneWeak& vOutSelectedQuery, iagpQueryZoneWeak vParent, uint32_t vDepth);
    bool m_DrawCircularFlameGraph(iagpQueryZonePtr vRoot, iagpQueryZoneWeak& vOutSelectedQuery, iagpQueryZoneWeak vParent, uint32_t vDepth);
};

class IAGP_API Profiler {
private:
    static constexpr uint32_t sMaxQueryCount = 1024U;
    static constexpr uint32_t sMaxDepth = 64U;

public:
    typedef std::function<bool(const char*, bool*, ImGuiWindowFlags)> ImGuiBeginFunctor;
    typedef std::function<void()> ImGuiEndFunctor;

#ifdef IAGP_VULKAN
    class CommandBufferInfos {
    private:
        vk::Device device;
        VulkanCoreWeak core;
        vk::QueryPool queryPool;
        Profiler* parentProfilerPtr = nullptr;

    public:
        std::array<vk::CommandBuffer, 2U> cmds;
        std::array<vk::Fence, 2U> fences;
        CommandBufferInfos() = default;
        ~CommandBufferInfos();
        void Init(VulkanCoreWeak vCore, vk::Device vDevice, vk::CommandPool vCmdPool, vk::QueryPool vQueryPool, Profiler* vParentProfilerPtr);
        void begin(const size_t& idx);
        void end(const size_t& idx);
        void writeTimeStamp(const size_t& idx, iagpQueryZoneWeak vQueryZone, vk::PipelineStageFlagBits vStages);
    };
#else
#endif

public:
    static ProfilerPtr create(VulkanCoreWeak vVulkanCore);
    static ProfilerPtr Instance(VulkanCoreWeak vVulkanCore = {}) {
        static auto _instance_ptr = Profiler::create(vVulkanCore);
        return _instance_ptr;
    };

private:
    iagpGraphTypeEnum m_GraphType = iagpGraphTypeEnum::HORIZONTAL;
    int32_t m_QueryZoneToClose = -1;
    ImGuiBeginFunctor m_ImGuiBeginFunctor =                                     //
        [](const char* vLabel, bool* pOpen, ImGuiWindowFlags vFlags) -> bool {  //
        return ImGui::Begin(vLabel, pOpen, vFlags);
    };
    ImGuiEndFunctor m_ImGuiEndFunctor = []() { ImGui::End(); };
    bool m_ShowDetails = false;
    bool m_IsLoaded = false;
    void* m_ThreadPtr = nullptr;
    ProfilerWeak m_This;
    VulkanCoreWeak m_VulkanCore;
    iagpQueryZonePtr m_RootZone = nullptr;
    iagpQueryZoneWeak m_SelectedQuery;                                  // query to show the flamegraph in this context
    std::array<iagpQueryZonePtr, sMaxQueryCount> m_QueryIDToZone = {};  // Get the zone for a query id because a query have to id's : start and end
    std::array<iagpQueryZonePtr, sMaxDepth> m_DepthToLastZone = {};     // last zone registered at this depth
    std::array<TimeStamp, sMaxQueryCount> m_TimeStampMeasures = {};
#ifdef IAGP_VULKAN
    vk::QueryPool m_QueryPool = {};
#else
#endif
    uint32_t m_QueryHead = 0U;
    uint32_t m_QueryCount = 0U;     // reseted each frames
    uint32_t m_MaxQueryCount = 0U;  // tuned at creation
    bool m_IsActive = false;
    bool m_IsPaused = false;

#ifdef IAGP_VULKAN
    std::unordered_map<std::string, CommandBufferInfos> m_CommandBuffers;
#else
#endif

    std::stack<iagpQueryZoneWeak> m_QueryStack;

    char m_TempBuffer[1024] = {};

public:
    Profiler() = default;
    ~Profiler() = default;

    bool Init(VulkanCoreWeak vVulkanCore);
    void Unit();
    void Clear();

    void DrawDetails(ImGuiWindowFlags vFlags = 0);
    void DrawDetailsNoWin();

    void DrawFlamGraph(const iagpGraphTypeEnum& vGraphType);
    void DrawFlamGraph(const char* vLabel, bool* pOpen, ImGuiWindowFlags vFlags = 0);
    void DrawFlamGraphNoWin();
    void DrawFlamGraphChilds(ImGuiWindowFlags vFlags = 0);

    void SetImGuiBeginFunctor(const ImGuiBeginFunctor& vImGuiBeginFunctor);
    void SetImGuiEndFunctor(const ImGuiEndFunctor& vImGuiEndFunctor);

    void Collect();

    bool& isActiveRef();
    const bool& isActive();
    bool& isPausedRef();
    const bool& isPaused();

    bool canRecordTimeStamp(const bool& isRoot = false);

    iagpQueryZonePtr GetQueryZoneForName(const void* vPtr, const std::string& vName, const std::string& vSection = "", const bool& vIsRoot = false);

    void BeginFrame(const char* vLabel);
    void EndFrame();

#ifdef IAGP_VULKAN
    bool beginChildZone(const VkCommandBuffer& vCmd, const void* vPtr, const std::string& vSection, const char* fmt, ...);
    bool endChildZone(const VkCommandBuffer& vCmd);

    void writeTimeStamp(const vk::CommandBuffer& vCmd, const size_t& idx, iagpQueryZoneWeak vQueryZone, vk::PipelineStageFlagBits vStages);

    CommandBufferInfos* beginChildZoneNoCmd(const void* vPtr, const std::string& vSection, const char* fmt, ...);
    void endChildZoneNoCmd(CommandBufferInfos* vCommandBufferInfosPtr);

    CommandBufferInfos* GetCommandBufferInfosPtr(const void* vPtr, const std::string& vSection, const char* fmt, ...);
    CommandBufferInfos* GetCommandBufferInfosPtr(const void* vPtr, const std::string& vSection, const char* fmt, va_list vArgs);
#else
#endif

private:
    void m_ClearMeasures();
    void m_AddMeasure();
#ifdef IAGP_VULKAN
    bool m_BeginZone(const VkCommandBuffer& vCmd, const bool& vIsRoot, const void* vPtr, const std::string& vSection, const char* label);
    bool m_BeginZone(const VkCommandBuffer& vCmd, const bool& vIsRoot, const void* vPtr, const std::string& vSection, const char* fmt, va_list vArgs);
    bool m_EndZone(const VkCommandBuffer& vCmd, const bool& vIsRoot);
#else
#endif

    void m_SetQueryZoneForDepth(iagpQueryZonePtr vQueryZone, uint32_t vDepth);
    iagpQueryZonePtr m_GetQueryZoneFromDepth(uint32_t vDepth);
    void m_DrawMenuBar();
    int32_t m_GetNextQueryId();
};

class IAGP_API vkScopedChildZone {
public:
    iagpQueryZonePtr queryZonePtr = nullptr;
#ifdef IAGP_VULKAN
    VkCommandBuffer commandBuffer = {};
    vk::PipelineStageFlagBits stages = vk::PipelineStageFlagBits::eBottomOfPipe;
#else
#endif

public:
#ifdef IAGP_VULKAN
    vkScopedChildZone(                             //
        const vk::PipelineStageFlagBits& vStages,  //
        const VkCommandBuffer& vCmd,               //
        const void* vPtr,                          //
        const std::string& vSection,               //
        const char* fmt,                           //
        ...);                                      //
    vkScopedChildZone(                             //
        const VkCommandBuffer& vCmd,               //
        const void* vPtr,                          //
        const std::string& vSection,               //
        const char* fmt,                           //
        ...);                                      //
#else
#endif
    ~vkScopedChildZone();
};

class IAGP_API vkScopedChildZoneNoCmd {
public:
    iagpQueryZonePtr queryZonePtr = nullptr;
#ifdef IAGP_VULKAN
    Profiler::CommandBufferInfos* infosPtr = nullptr;
    vk::PipelineStageFlagBits stages = vk::PipelineStageFlagBits::eBottomOfPipe;
#else
#endif

public:
#ifdef IAGP_VULKAN
    vkScopedChildZoneNoCmd(                        //
        const vk::PipelineStageFlagBits& vStages,  //
        const void* vPtr,                          //
        const std::string& vSection,               //
        const char* fmt,                           //
        ...);                                      //
#else
#endif
    vkScopedChildZoneNoCmd(                        //
        const void* vPtr,                          //
        const std::string& vSection,               //
        const char* fmt,                           //
        ...);                                      //
    ~vkScopedChildZoneNoCmd();
};

}  // namespace GaiApi
