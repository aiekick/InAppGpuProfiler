#version 330
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aUV;
layout(location = 0) out vec2 vUV;

void main() 
{
	vUV = aUV;
	gl_Position = vec4(aPosition, 0.0, 1.0);
}
