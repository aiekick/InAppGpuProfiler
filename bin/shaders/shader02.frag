#version 330
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec2 vUV;

// https://www.shadertoy.com/view/4tX3R4
// Created by Stephane Cuillerdier - @Aiekick/2014

uniform float iTime;
uniform vec3 iResolution;

float metaline(vec2 p, vec2 o, float thick, vec2 l)
{
    vec2 po = 2.*p+o;
    return thick / dot(po,vec2(l.x,l.y));
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    float speed = 0.3;
    float t0 = iTime*speed;
    float t1 = sin(t0);
    float t2 = 0.5*t1+0.5;
    float zoom=25.;
    float ratio = iResolution.x/iResolution.y;
	vec2 uv = fragCoord.xy / iResolution.xy*2.-1.;uv.x*=ratio;uv*=zoom;

	// cadre
    float thick=0.5;
    float inv=1.;
	float bottom = metaline(uv,vec2(0.,2.)*zoom, thick, vec2(0.0,1.*inv));
	float top = metaline(uv,vec2(0.,-2.)*zoom, thick, vec2(0.0,-1.*inv));
	float left = metaline(uv,vec2(2.*ratio,0.)*zoom, 0.5, vec2(1.*inv,0.0));
	float right = metaline(uv,vec2(-2.*ratio,0.)*zoom, 0.5, vec2(-1.*inv,0.0));
	float rect=bottom+top+left+right;
    
    // uv / mo
    vec2 uvo = uv;//-mo;
    float phase=1.1;
    float tho = length(uvo)*phase+t1;
    float thop = t0*20.;
    
    // map spiral
   	uvo+=vec2(tho*cos(tho-1.25*thop),tho*sin(tho-1.15*thop));
    
    // metaball
    float mbr = 8.;
    float mb = mbr / dot(uvo,uvo);

	//display
    float d0 = mb+rect;
    
    float d = smoothstep(d0-2.,d0+1.2,1.);
    
	float r = mix(1./d, d, 1.);
    float g = mix(1./d, d, 3.);
    float b = mix(1./d, d, 5.);
    vec3 c = vec3(r,g,b);
    
    fragColor.rgb = c;
}

void main() {
	fragColor =  vec4(0,0,0,1);
    mainImage(fragColor, gl_FragCoord.xy);
    fragColor.a = 1.0;
}
