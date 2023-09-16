cmake_minimum_required(VERSION 3.5)

project(InAppGpuProfiler)

add_library(InAppGpuProfiler STATIC
    InAppGpuProfiler.cpp
    InAppGpuProfiler.h
    InAppGpuProfilerConfig.h
)

target_include_directories(InAppGpuProfiler PRIVATE 
	${CMAKE_CURRENT_SOURCE_DIR})
