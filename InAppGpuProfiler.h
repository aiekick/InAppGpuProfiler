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
#include <memory>
#include <string>
#include <unordered_map>
#include <glad/glad.h>

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

#ifndef GPU_CONTEXT
#define GPU_CONTEXT void*
#endif // GPU_CONTEXT

namespace iagp {

class InAppGpuQueryZone;
typedef std::shared_ptr<InAppGpuQueryZone> IAGPQueryZonePtr;

class InAppGpuGLContext;
typedef std::shared_ptr<InAppGpuGLContext> IAGPContextPtr;

class InAppGpuAverageValue {
private:
    static constexpr uint32_t sCountAverageValues = 60U;
    double m_PerFrame[sCountAverageValues] = {};
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
    GPU_CONTEXT m_Context;
    std::string m_BarLabel;
    std::string m_SectionName;
    float m_BarWidth = 0.0f;
    float m_BarPos = 0.0f;

public:
    InAppGpuQueryZone(GPU_CONTEXT vContext, const std::string& vName, const std::string& vSectionName, const bool& vIsRoot = false);
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
    GPU_CONTEXT m_Context;
    IAGPQueryZonePtr m_RootZone = nullptr;
    std::unordered_map<GLuint, IAGPQueryZonePtr> m_QueryIDToZone;    // Get the zone for a query id because a query have to id's : start and end
    std::unordered_map<GLuint, IAGPQueryZonePtr> m_DepthToLastZone;  // last zone registered at this puDepth
    std::set<GLuint> m_PendingUpdate;                                // some queries msut but retrieveds

public:
    InAppGpuGLContext(GPU_CONTEXT vContext);
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
    bool puIsActive = false;
    bool puIsPaused = false;

private:
    std::unordered_map<intptr_t, IAGPContextPtr> m_Contexts;

public:
    void Clear();
    void Init();
    void Unit();
    void Collect();
    void Draw();
    IAGPContextPtr GetContextPtr(GPU_CONTEXT vContext);

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
