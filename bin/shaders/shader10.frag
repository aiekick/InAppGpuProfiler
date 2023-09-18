#version 330
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec2 vUV;

// shader https://www.shadertoy.com/view/4djBDR
// Created by Stephane Cuillerdier - @Aiekick/2014

uniform int iFrame;
uniform float iTime;
uniform vec3 iResolution;
uniform sampler2D iChannel0; // back to front

void main() {
    vec2 g = gl_FragCoord.xy;
    vec4 f = vec4(iResolution, 1);
    vec2 v = (g+g-f.xy)/f.y*3.;
    f *= texture(iChannel0, g/f.xy) / length(f); // read back
    g = vec2(iFrame + 2, iFrame);
    g = v - sin(g) * fract(iTime*.1 + 4.*sin(g))*3.;
    f += .1 / max(abs(g.x), g.y);
	fragColor = f; // write front
    fragColor.a = 1.0;
}
