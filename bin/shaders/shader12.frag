#version 330
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec2 vUV;

uniform float iTime;

void main() {
	float t = sin(iTime) * 0.5 + 0.5;
	fragColor =  vec4(vUV,t,1);
}
