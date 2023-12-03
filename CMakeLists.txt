cmake_minimum_required(VERSION 3.5)

set(PROJECT InAppGpuProfiler)
set(PROJECT_VER 1.90.1)

project(
	${PROJECT} 
	VERSION ${PROJECT_VER} 
	LANGUAGES CXX)

if(USE_SHARED_LIBS)
	set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
	set(LLVM_USE_CRT_DEBUG MDd CACHE STRING "" FORCE)
	set(LLVM_USE_CRT_MINSIZEREL MD CACHE STRING "" FORCE)
	set(LLVM_USE_CRT_RELEASE MD CACHE STRING "" FORCE)
	set(LLVM_USE_CRT_RELWITHDEBINFO MD CACHE STRING "" FORCE)
	set(USE_MSVC_RUNTIME_LIBRARY_DLL ON CACHE BOOL "" FORCE)
else()
	set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
	set(LLVM_USE_CRT_DEBUG MTd CACHE STRING "" FORCE)
	set(LLVM_USE_CRT_MINSIZEREL MT CACHE STRING "" FORCE)
	set(LLVM_USE_CRT_RELEASE MT CACHE STRING "" FORCE)
	set(LLVM_USE_CRT_RELWITHDEBINFO MT CACHE STRING "" FORCE)
	set(USE_MSVC_RUNTIME_LIBRARY_DLL OFF CACHE BOOL "" FORCE)
endif()

if(NOT CMAKE_DEBUG_POSTFIX)
  set(CMAKE_DEBUG_POSTFIX d)
endif()

#####################################################
#####################################################

## for group smake targets in the dir cmakeTargets
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "cmakeTargets")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")

add_definitions(-DIMGUI_DEFINE_MATH_OPERATORS)
#add_definitions(-DIMGUI_DISABLE_OBSOLETE_FUNCTIONS)

#####################################################
#####################################################

file(GLOB IMGUI_INAPPGPUPROFILER_SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/*.h
		${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

#####################################################
#####################################################

if (BUILD_SHARED_LIBS)
	set(USE_MSVC_RUNTIME_LIBRARY_DLL ON CACHE BOOL "" FORCE)
	add_library(${PROJECT} ${IMGUI_INAPPGPUPROFILER_SOURCES})
	target_compile_definitions(${PROJECT} INTERFACE BUILD_IAGP_SHARED_LIBS)
	set_target_properties(${PROJECT} PROPERTIES POSITION_INDEPENDENT_CODE ON)
else()
	set(USE_MSVC_RUNTIME_LIBRARY_DLL OFF CACHE BOOL "" FORCE)
	add_library(${PROJECT} STATIC ${IMGUI_INAPPGPUPROFILER_SOURCES})
endif()

target_include_directories(${PROJECT} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
)

if(UNIX)
    target_compile_options(${PROJECT} PUBLIC "-Wno-unknown-pragmas -Wno-unused-variable -Wno-unused-parameter")
endif()

target_compile_definitions(${PROJECT} PUBLIC USE_IN_APP_GPU_PROFILER)
 
if(USE_DEBUG_SANITIZER)
	target_compile_options(${PROJECT} PRIVATE $<$<CONFIG:Debug>:-fsanitize=address -static-libasan -static-libasan>)
	target_link_options(${PROJECT} PRIVATE $<$<CONFIG:Debug>:-fsanitize=address -static-libasan>)
	message("Address Sanitizer enabled for projet ${PROJECT}")
endif()

## target_link_libraries(${PROJECT}
## 	${GAIA_LIBRARIES}
## )

install(TARGETS ${PROJECT} RUNTIME DESTINATION / COMPONENT APP_LIBS_${TINYXML2_LIBRARIES})
install(TARGETS ${TINYXML2_LIBRARIES} RUNTIME DESTINATION / COMPONENT APP_LIBS_${TINYXML2_LIBRARIES})
install(TARGETS ${FREETYPE_LIBRARIES} RUNTIME DESTINATION / COMPONENT APP_LIBS_${FREETYPE_LIBRARIES})

