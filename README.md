[![Win](https://github.com/aiekick/InAppGpuProfiler/actions/workflows/Win.yml/badge.svg)](https://github.com/aiekick/InAppGpuProfiler/actions/workflows/Win.yml)
[![Linux](https://github.com/aiekick/InAppGpuProfiler/actions/workflows/Linux.yml/badge.svg)](https://github.com/aiekick/InAppGpuProfiler/actions/workflows/Linux.yml)
[![Osx](https://github.com/aiekick/InAppGpuProfiler/actions/workflows/Osx.yml/badge.svg)](https://github.com/aiekick/InAppGpuProfiler/actions/workflows/Osx.yml)
[![Wrapped Dear ImGui Version](https://img.shields.io/badge/Dear%20ImGui%20Version-1.89.9-blue.svg)](https://github.com/ocornut/imgui)

# [InAppGpuProfiler](https://github.com/aiekick/InAppGpuProfiler)

# purpose

This is the In App Gpu Profiler im using since two years in [NoodlesPlate](https://github.com/aiekick/NoodlesPlate).

Only Opengl is supported for the moment

## ImGui Supported Version

InAppGpuProfiler follow the master and docking branch of ImGui . currently ImGui 1.89.9

## Requirements:

A opengl Loader (like glad)
And of course, have added [Dear ImGui](https://github.com/ocornut/imgui) to your project for this to work at all.

# Features

- Compatible with MacOs, Linux, Windows
- Scopped queries functions
- can open profiling section in sub windows
- can open profiling section in the same windows and get a breadcrumb trail to go basck to parents

## Warnings : 
- the circular vizualization is in work in progress state. dont use it for the moment

## to do :
- C api

# how to use it

## Mandatory definition

InAppGpuProfiler is designed to be not invasive with you build configuration

So you need to create a config.

* You have a file "InAppGpuProfilerConfig.h" ready for that in the lib directory
* You can also create your own custom file and link it by defining the symbol CUSTOM_IN_APP_GPU_PROFILER_CONFIG

  this is what is done in the demo app by this cmake line :
  ```
  add_definitions(-DCUSTOM_IN_APP_GPU_PROFILER_CONFIG="${CMAKE_SOURCE_DIR}/CustomInAppGpuProfiler.h")
  ```

In this config you have MANDATORY things to define :
- the include of your opengl loader.
- the include of imgui.h and imgui_internal.h
- define he fucntion to get/set the opengl context
- define the opengl context pointer used by you get/set current context functions

**If you dont define theses mandatory things, you will have linker issues or the profiler will not be able to catch gpu metrics**

you can check the demo app for a typical use with the couple glfw/glad
 
## What to do in code

You must call 'AIGPNewFrame' one time per frame. 
This must be the root of your gpu metric scope

ex :
```cpp
AIGPNewFrame("GPU Frame", "GPU Frame");  // a main Zone is always needed                
```

then you msut call 'AIGPScoped' for each zones you want to measure

ex :
```cpp
AIGPScoped("render_imgui", "ImGui");
{
	AIGPScoped("Opengl", "glViewport");
	glViewport(0, 0, display_w, display_h);
}

{
	AIGPScoped("Opengl", "glClearColor");
	glClearColor(0.3f, 0.3f, 0.3f, 0.3f);
}

{
	AIGPScoped("Opengl", "glClear");
	glClear(GL_COLOR_BUFFER_BIT);
}

{
	AIGPScoped("ImGui", "RenderDrawData");
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}     
```

you can also use the variadic form :

```cpp
AIGPScoped("Opengl", "glGenerateMipmap %u", m_TexId);
```

these functions must be included in a scope.

you cant put two function in the same scope since the first metric measure 

will be done on constructor of AIGPScoped and the second metric measure 

will be done on destructor of AIGPScoped

finally you must collect all gpu metrics out of the scope of 'AIGPNewFrame' 

by calling 'AIGPCollect' one time per frame

ex :
```cpp
AIGPCollect;  // collect all measure queries out of Main Frame               
```

full sample from the DemoApp :

```cpp 
while (!glfwWindowShouldClose(window)) {
    {
        AIGPNewFrame("GPU Frame", "GPU Frame");  // a main Zone is always needed
	glfwMakeContextCurrent(window);

	render_shaders(); // many AIGPScoped are caleed in this function

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// Cpu Zone : prepare
	calc_imgui();
	ImGui::Render();

	// GPU Zone : Rendering
	glfwMakeContextCurrent(window);
	{
            AIGPScoped("render_imgui", "ImGui");
            glViewport(0, 0, display_w, display_h);
            glClearColor(0.3f, 0.3f, 0.3f, 0.3f);
            glClear(GL_COLOR_BUFFER_BIT);

            {
	        AIGPScoped("ImGui", "RenderDrawData");
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            }
	}

	{
            AIGPScoped("Opengl", "glfwSwapBuffers");
            glfwSwapBuffers(window);
	}
    }

    AIGPCollect;  // collect all measure queries out of Main Frame
}      
```

that's all folks :)

# Feature : BreadCrumb Trail (fil d'ariane)

By left clicking on a bar, you can open it int the main profiler window. 

When you click on buttons of the breadcrumb trail you can select a parent bar to show

![img](https://github.com/aiekick/InAppGpuProfiler/blob/DemoApp/doc/breadcrumbtrail.gif)

# Feature : Sub Windows per profiler bars

By right clicking on a bars, you can open the bar in another window.

![img](https://github.com/aiekick/InAppGpuProfiler/blob/DemoApp/doc/sub_windows.gif)

# The Demo App

The demo app let you see how to use in detail the Profiler

Its coming with a little opengl framework with some of my [shaders](https://www.shadertoy.com/user/aiekick)

![img](https://github.com/aiekick/InAppGpuProfiler/blob/DemoApp/doc/thumbnail.png)
