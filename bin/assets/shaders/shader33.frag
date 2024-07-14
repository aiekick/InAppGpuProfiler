#version 330
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec2 vUV;

uniform float iTime;
uniform vec3 iResolution;
uniform sampler2D iChannel0;

// https://www.shadertoy.com/view/ltK3WD
// Created by Stephane Cuillerdier - Aiekick/2016 (twitter:@aiekick)

// based on Triangular Voronoi https://www.shadertoy.com/view/ltKGWD
void main()
{
	vec4 f = vec4(0);
	vec2 g = gl_FragCoord.xy;
	vec2 p = g /= iResolution.y / 5.; f.x=9.;
    
    float t = iTime * 0.1;
    
    for(int x=-2;x<=2;x++)
    for(int y=-2;y<=2;y++)
    {	
        p = vec2(x,y);
		p += .5 + .5*sin( t * 10. + 9. * fract(sin((floor(g)+p)*mat2(2,5,5,2)))) - fract(g);
        //p *= mat2(cos(t), -sin(t), sin(t), cos(t));
        f.y = max(abs(p.x)*.866 - p.y*.5, p.y);
        if (f.y < f.x)
        {
            f.x = f.y;
            f.zw = p;
        }
    }
	
    vec3 n = vec3(0);
    
    if ( (f.x - (-f.z*.866 - f.w*.5)) 	<.0001) 	n = vec3(1,0,0);
	if ( (f.x - (f.z*.866 - f.w*.5))	<.0001) 	n = vec3(0,1,0);
	if ( (f.x - f.w)					<.0001) 	n = vec3(0,0,1);
	
	const float _pi = radians(180.0);
	const float _2pi = radians(360.0);

    // longlat
	vec3 rd = normalize(-n);
	float theta = atan(rd.x,rd.z);
	float phi =  asin(rd.y);
	vec2 uv = 0.5 + vec2(-theta / _2pi, -phi / _pi);
    fragColor = sqrt(texture(iChannel0, uv) * f.x * 1.5);
    fragColor.a = 1.0;
}