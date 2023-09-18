#version 330
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor0;
layout(location = 1) out vec4 fragColor1;
layout(location = 0) in vec2 vUV;

uniform float iTime;
uniform int iFrame;
uniform vec3 iResolution;
uniform sampler2D iChannel0;

// https://www.shadertoy.com/view/4sfczN
// Created by Stephane Cuillerdier - Aiekick/2017 (twitter:@aiekick)

/* feed rate */ 				#define fr .01131
/* kill rate */ 				#define kr .04
/* laplacian corner ratio */	#define lc .2
/* laplacian side ratio */ 		#define ls .8
/* key space */ 				#define kbd_space 32

vec4 get(sampler2D sam, vec2 g, vec2 s, float x, float y)
{
    vec2 v = g + vec2(x,y);
    if (v.x < 0.) v.x = s.x;
    if (v.y < 0.) v.y = s.y;
    if (v.x > s.x) v.x = 0.;
    if (v.y > s.y) v.y = 0.;
	return texture(sam, v / s);
}

vec4 frag(sampler2D sam, vec2 g, vec2 s, int i)
{
    vec4 l 	= 	get(sam, g, s, -1. ,  0.);
	vec4 lt = 	get(sam, g, s, -1. ,  1.);
	vec4 t 	= 	get(sam, g, s,  0. ,  1.);
	vec4 rt = 	get(sam, g, s,  1. ,  1.);
	vec4 r 	= 	get(sam, g, s,  1. ,  0.);
	vec4 rb = 	get(sam, g, s,  1. , -1.);
	vec4 b 	= 	get(sam, g, s,  0. , -1.);
	vec4 lb = 	get(sam, g, s, -1. , -1.);
	vec4 c 	= 	get(sam, g, s,  0. ,  0.);
	vec4 lap = (l+t+r+b)/4.*ls + (lt+rt+rb+lb)/4.*lc - c; // laplacian

	float re = c.x * c.y * c.y; // reaction
    c.xy += lap.xy + vec2(fr * (1. - c.x) - re, re - (fr + kr) * c.y); // grayscott formula
	
	if (i < 2)	c = vec4(1,0,0,1);
	if (length(g - s * 0.5) < 5.) c = vec4(0,1,0,1);
        
	return vec4(clamp(c.xy, 0., 1e1), 0, 1);
}

void main() {
    vec2 g = gl_FragCoord.xy;
    vec2 s = iResolution.xy;
    fragColor1 = frag(iChannel0, g, s, iFrame);

    vec2 v = (g+g-s)/s.y;
    float a = atan(v.x,v.y) + length(v) * 4. - iTime * 2.;
    float cc = get(iChannel0, g, s, 0., 0.).r;
    float cc2 = get(iChannel0, g, s, cos(a) * 0.8, sin(a) * 0.8).r;
    fragColor0 = vec4(cc*cc);
    fragColor0 += vec4(.5, .8, 1,1) * max(cc2 * cc2 * cc2 - cc * cc * cc, 0.0) * s.y * 0.2;
    fragColor0 = fragColor0.rgba * .2 + fragColor0.grba * .3;
    fragColor0.a = 1.0;
}
