#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 aPosition;
layout(location = 0) out vec2 v_uv;

void main() 
{
	v_uv = aPosition;
	gl_Position = vec4(aPosition, 0.0, 1.0);
}
