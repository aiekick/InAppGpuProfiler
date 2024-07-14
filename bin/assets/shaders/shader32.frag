#version 330
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec2 vUV;

uniform float iTime;
uniform int iFrame;
uniform vec3 iResolution;
uniform vec4 iMouse;
uniform sampler2D iChannel0;

// https://www.shadertoy.com/view/XlSSzV
// Created by Stephane Cuillerdier - Aiekick/2015
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// Tuned via XShade (http://www.funparadigm.com/xshade/)

/* 
	variation more cloudy off Another Cloudy Tunnel : 
		https://www.shadertoy.com/view/4lSXRK

	the cloudy famous tech come from the shader of duke : https://www.shadertoy.com/view/MljXDw
        Himself a Port of a demo by Las => http://www.pouet.net/topic.php?which=7920&page=29&x=14&y=9
*/

float t;

float cosPath(vec3 p, vec3 dec){return dec.x * cos(p.z * dec.y + dec.z);}
float sinPath(vec3 p, vec3 dec){return dec.x * sin(p.z * dec.y + dec.z);}

vec2 getCylinder(vec3 p, vec2 pos, float r, vec3 c, vec3 s)
{
	return p.xy - pos - vec2(cosPath(p, c), sinPath(p, s));
}

/////////////////////////
// FROM Shader Cloudy spikeball from duke : https://www.shadertoy.com/view/MljXDw
float pn( in vec3 x )
{
    vec3 p = floor(x);
    vec3 fragColor = fract(x);
	fragColor = fragColor*fragColor*(3.0-2.0*fragColor);
	vec2 uv = (p.xy+vec2(37.0,17.0)*p.z) + fragColor.xy;
	vec2 rg = texture(iChannel0, (uv+ 0.5)/256.0, -100.0 ).yx;
	return -1.0+2.4*mix( rg.x, rg.y, fragColor.z );
}

float fpn(vec3 p) 
{
    p += t*5.;
	return pn(p*0.02)*1.98 + pn(p*0.02)*0.62 + pn(p*0.09)*0.39;
}
/////////////////////////

float map(vec3 p)
{
	float pnNoise = fpn(p*13.)*.8;
	float path = sinPath(p ,vec3(6.2, .33, 0.));
	float bottom = p.y + pnNoise;
	float cyl = 0.;vec2 vecOld;
	for (float i=0.;i<6.;i++)
	{
		float x = 1. * i;
		float y	= .88 + 0.0102*i;
		float z	 = -0.02 -0.16*i;
		float r = 4.4 + 2.45 * i;
		vec2 vec = getCylinder(p, vec2(path, 3.7 * i), r , vec3(x,y,z), vec3(z,x,y));
		cyl = r - min(length(vec), length(vecOld));
		vecOld = vec;	
	}
	cyl += pnNoise;
	cyl = min(cyl, bottom);
	return cyl;
}

vec3 cam(vec2 uv, vec3 ro, vec3 cu, vec3 cv)
{
	vec3 rov = normalize(cv-ro);
    vec3 u =  normalize(cross(cu, rov));
    vec3 v =  normalize(cross(rov, u));
	float fov = 3.;
    vec3 rd = normalize(rov + fov*u*uv.x + fov*v*uv.y);
    return rd;
}

void main()
{
    t = iTime*2.5;
	fragColor = vec4(0,0.15,0.32,1);
	vec2 si = iResolution.xy;
	vec2 g = gl_FragCoord.xy;
	vec2 uv = (2.*g-si)/min(si.x, si.y);
    vec3 ro = vec3(0), p=ro;
	ro.y = sin(t*.2)*15.+15.;
	ro.x = sin(t*.5)*5.;
	ro.z = t*5.;
	vec3 rd = cam(uv, p, vec3(0,1,0), p + vec3(0,0,1));
	float s = 1., h = .15, td = 0., d=1.,dd=0., w;
    float var = 0.03;
    if (iMouse.z>0.) var = 0.1*iMouse.y/iResolution.y;
    for(float i=0.;i<200.;i++)
	{      
		if(s<0.01||d>500.||td>.95) break;
        s = map(p) * (s>0.001?var:.2);
		if (s < h)
		{
			w = (1.-td) * (h-s)*i/200.;
			fragColor += w;
			td += w;
		}
		dd += 0.012;
		td += 0.005;
		s = max(s, 0.05);
		d+=s;	
		p = ro+rd*d;	
   	}
	fragColor.rgb = mix( fragColor.rgb, vec3(0,0.15,0.52), 1.0 - exp( -0.001*d*d) )/dd; // fog
	
	// vigneting from iq Shader Mike : https://www.shadertoy.com/view/MsXGWr
    vec2 q = g/si;
    fragColor.rgb *= 0.5 + 0.5*pow( 16.0*q.x*q.y*(1.0-q.x)*(1.0-q.y), 0.25 );
}