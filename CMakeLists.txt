cmake_minimum_required(VERSION 3.5)

project(InAppGpuProfiler)

if (BUILD_SHARED_LIBS)
	set(USE_MSVC_RUNTIME_LIBRARY_DLL ON CACHE BOOL "" FORCE)
	add_library(InAppGpuProfiler 
		${CMAKE_CURRENT_SOURCE_DIR}/InAppGpuProfiler.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/InAppGpuProfiler.h
		${CMAKE_CURRENT_SOURCE_DIR}/InAppGpuProfilerConfig.h
	)
	target_compile_definitions(InAppGpuProfiler INTERFACE IN_APP_GPU_PROFILER_API)
	set_target_properties(InAppGpuProfiler PROPERTIES POSITION_INDEPENDENT_CODE ON)
else()
	set(USE_MSVC_RUNTIME_LIBRARY_DLL OFF CACHE BOOL "" FORCE)
	add_library(InAppGpuProfiler STATIC 
		${CMAKE_CURRENT_SOURCE_DIR}/InAppGpuProfiler.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/InAppGpuProfiler.h
		${CMAKE_CURRENT_SOURCE_DIR}/InAppGpuProfilerConfig.h
	)
endif()

target_include_directories(InAppGpuProfiler PRIVATE 
	${CMAKE_CURRENT_SOURCE_DIR})
