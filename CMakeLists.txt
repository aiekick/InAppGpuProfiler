cmake_minimum_required(VERSION 3.20)

set(PROJECT iagp)

enable_language(C CXX)

project(
	${PROJECT} 
	LANGUAGES CXX
)

if(USE_SHARED_LIBS)
	set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
	set(LLVM_USE_CRT_DEBUG MDd CACHE STRING "" FORCE)
	set(LLVM_USE_CRT_MINSIZEREL MD CACHE STRING "" FORCE)
	set(LLVM_USE_CRT_RELEASE MD CACHE STRING "" FORCE)
	set(LLVM_USE_CRT_RELWITHDEBINFO MD CACHE STRING "" FORCE)
else()
	set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
	set(LLVM_USE_CRT_DEBUG MTd CACHE STRING "" FORCE)
	set(LLVM_USE_CRT_MINSIZEREL MT CACHE STRING "" FORCE)
	set(LLVM_USE_CRT_RELEASE MT CACHE STRING "" FORCE)
	set(LLVM_USE_CRT_RELWITHDEBINFO MT CACHE STRING "" FORCE)
endif()

if(NOT CMAKE_DEBUG_POSTFIX)
  set(CMAKE_DEBUG_POSTFIX _debug)
endif()
if(NOT CMAKE_RELEASE_POSTFIX)
  set(CMAKE_RELEASE_POSTFIX)
endif()
if(NOT CMAKE_MINSIZEREL_POSTFIX)
  set(CMAKE_MINSIZEREL_POSTFIX _minsizerel)
endif()
if(NOT CMAKE_RELWITHDEBINFO_POSTFIX)
  set(CMAKE_RELWITHDEBINFO_POSTFIX _reldeb)
endif()

if (BUILD_SHARED_LIBS)
	set(USE_MSVC_RUNTIME_LIBRARY_DLL ON CACHE BOOL "" FORCE)
	add_library(${PROJECT} 
		${CMAKE_CURRENT_SOURCE_DIR}/iagp.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/iagp.h
		${CMAKE_CURRENT_SOURCE_DIR}/iagpConfig.h
	)
	target_compile_definitions(${PROJECT} INTERFACE BUILD_IN_APP_GPU_PROFILER_SHARED_LIBS)
	set_target_properties(${PROJECT} PROPERTIES POSITION_INDEPENDENT_CODE ON)
else()
	set(USE_MSVC_RUNTIME_LIBRARY_DLL OFF CACHE BOOL "" FORCE)
	add_library(${PROJECT} STATIC 
		${CMAKE_CURRENT_SOURCE_DIR}/iagp.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/iagp.h
		${CMAKE_CURRENT_SOURCE_DIR}/iagpConfig.h
	)
endif()

target_include_directories(${PROJECT} PRIVATE 
	${CMAKE_CURRENT_SOURCE_DIR})
	
if(UNIX)
    target_compile_options(${PROJECT} PUBLIC "-Wno-unknown-pragmas -Wno-unused-variable -Wno-unused-parameter")
endif()

if(USE_DEBUG_SANITIZER)
	target_compile_options(${PROJECT} PRIVATE $<$<CONFIG:Debug>:-fsanitize=address -static-libasan -static-libasan>)
	target_link_options(${PROJECT} PRIVATE $<$<CONFIG:Debug>:-fsanitize=address -static-libasan>)
	message("Address Sanitizer enabled for projet ${PROJECT}")
endif()

set_target_properties(${PROJECT} PROPERTIES OUTPUT_NAME "iagp")

set(IN_APP_GPU_PROFILER_INCLUDE_DIRS ${IN_APP_GPU_PROFILER_INCLUDE_DIRS} PARENT_SCOPE)
set(IN_APP_GPU_PROFILER_LIBRARIES ${PROJECT} PARENT_SCOPE)
