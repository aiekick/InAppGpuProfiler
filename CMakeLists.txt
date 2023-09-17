cmake_minimum_required(VERSION 3.5)

set(PROJECT Demo)

enable_language(C CXX)
project(${PROJECT} CXX)

option(USE_NEW_IMGUI_KEY_SYSTEM "Define the symbol IMGUI_DISABLE_OBSOLETE_KEYIO" ON)

include(cmake/3rdparty.cmake)

if(MSVC)
    add_definitions(-DMSVC)
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wdeprecated-declarations -Wunused-parameter")
endif ()

add_definitions(${GLFW_DEFINITIONS})
add_definitions(-DCUSTOM_IMGUIFILEDIALOG_CONFIG="../../../CustomImGuiFileDialogConfig.h")
add_definitions(-D_CRT_SECURE_NO_WARNINGS)

if (USE_NEW_IMGUI_KEY_SYSTEM)
	add_definitions(-DIMGUI_DISABLE_OBSOLETE_KEYIO)
endif()

file(GLOB MAIN_SOURCES 
	${CMAKE_SOURCE_DIR}/main.cpp
	${CMAKE_SOURCE_DIR}/fbo.hpp
	${CMAKE_SOURCE_DIR}/shader.hpp
	${CMAKE_SOURCE_DIR}/texture.hpp
	${CMAKE_SOURCE_DIR}/program.hpp
	${CMAKE_SOURCE_DIR}/glApi.hpp)
file(GLOB IMGUIIMPL_SOURCES 
		${CMAKE_SOURCE_DIR}/3rdparty/imgui/backends/imgui_impl_glfw.cpp
		${CMAKE_SOURCE_DIR}/3rdparty/imgui/backends/imgui_impl_glfw.h
		${CMAKE_SOURCE_DIR}/3rdparty/imgui/backends/imgui_impl_opengl3.cpp
		${CMAKE_SOURCE_DIR}/3rdparty/imgui/backends/imgui_impl_opengl3.h)
file(GLOB SHADERS_SOURCES 
		${CMAKE_SOURCE_DIR}/bin/shaders/*.vert
		${CMAKE_SOURCE_DIR}/bin/shaders/*.frag)
file(GLOB GL_API_SOURCES 
		${CMAKE_SOURCE_DIR}/glAPi/*.*)
source_group(main FILES ${MAIN_SOURCES})
source_group(ImGuiImpl FILES ${IMGUIIMPL_SOURCES})
source_group(Shaders FILES ${SHADERS_SOURCES})
source_group(glApi FILES ${GL_API_SOURCES})

include_directories(
	${PLOG_INCLUDE_DIR}
	${GLM_INCLUDE_DIR}
	${OPENGL_INCLUDE_DIR}
	${GLFW_INCLUDE_DIR}
	${GLAD_INCLUDE_DIR}
	${IMGUI_INCLUDE_DIR}
	${CMAKE_SOURCE_DIR}
)

add_subdirectory(${CMAKE_SOURCE_DIR}/InAppGpuProfiler)
	
add_executable(${PROJECT}
	${MAIN_SOURCES}
	${CUSTOM_SOURCES}
	${GL_API_SOURCES}
	${SHADERS_SOURCES}
	${IMGUIIMPL_SOURCES}
)

if(WIN32)
	set_target_properties(${PROJECT} PROPERTIES OUTPUT_NAME "${PROJECT}_Win32")
elseif(UNIX)
	set_target_properties(${PROJECT} PROPERTIES OUTPUT_NAME "${PROJECT}_Unix")
endif()

set(CMAKE_CXX_STANDARD 11)
set_target_properties(${PROJECT} PROPERTIES CXX_STANDARD 11)
set_target_properties(InAppGpuProfiler PROPERTIES CXX_STANDARD 11)
	
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}")
install(TARGETS ${PROJECT} DESTINATION ${CMAKE_INSTALL_PREFIX})

target_link_libraries(${PROJECT} PRIVATE
	${OPENGL_LIBRARIES}
	${GLFW_LIBRARIES}
	${GLAD_LIBRARIES}
	${IMGUI_LIBRARIES}
	InAppGpuProfiler
)

set_target_properties(InAppGpuProfiler PROPERTIES FOLDER Lib)

target_include_directories(InAppGpuProfiler PRIVATE
	${CMAKE_SOURCE_DIR}
)

set(FINAL_BIN_DIR ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_INSTALL_PREFIX "${FINAL_BIN_DIR}")
set(CMAKE_PDB_OUTPUT_DIRECTORY "${FINAL_BIN_DIR}/debug")

set_target_properties(${PROJECT} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${FINAL_BIN_DIR}")
set_target_properties(${PROJECT} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${FINAL_BIN_DIR}")
set_target_properties(${PROJECT} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${FINAL_BIN_DIR}")
set_target_properties(${PROJECT} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${FINAL_BIN_DIR}")
 