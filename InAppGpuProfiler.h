// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2017-2023 Stephane Cuillerdier All rights reserved

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
#define GLPNewFrame(section, fmt, ...)                                               \
    auto __IAGP__ScopedMainZone = iagp::InAppGpuScopedZone(true, section, fmt, ##__VA_ARGS__); \
    (void)__IAGP__ScopedMainZone
#define GLPScoped(section, fmt, ...)                                                 \
    auto __IAGP__ScopedSubZone = iagp::InAppGpuScopedZone(false, section, fmt, ##__VA_ARGS__); \
    (void)__IAGP__ScopedSubZone

namespace iagp {

class InAppGpuQueryZone;
typedef std::shared_ptr<InAppGpuQueryZone> IAGPQueryZonePtr;
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

class InAppGpuGLContext;
typedef std::shared_ptr<InAppGpuGLContext> IAGPContextPtr;

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
    IAGPContextPtr GetContextPtr(GPU_CONTEXT_PTR vThread);

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
