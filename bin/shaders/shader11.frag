#version 330
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec2 vUV;

// https://www.shadertoy.com/view/WdSGzt
// Created by Stephane Cuillerdier - Aiekick/2019 (twitter:@aiekick)

uniform float iTime;
uniform vec3 iResolution;

#define _c vec2(0.9240,0)
#define _niter 100
#define _k 0.25912
#define _scale 2.2
#define _limit 0.03
#define _dist 8.
#define _color vec3(1,0,1)
#define _colorVar vec2(3,1.51)

vec2 zmul(vec2 a, vec2 b){return mat2(a,-a.y,a.x)*b;} // z * z 
vec2 zinv(vec2 a){return vec2(a.x, -a.y) / dot(a,a);} // 1 / z

const float AA = 2.;
    
float shape(vec2 z)
{
	//return max(abs(z.x), abs(z.y)) * 0.8 + dot(z,z) * 0.2;
	//return max(abs(z.x)-z.y,z.y);
	return dot(z,z);
}

void mainImage( out vec4 f, in vec2 g )
{
    f = vec4(0);
    
	vec2 si = iResolution.xy;
        
    for( float m=0.; m<AA; m++ )
    for( float n=0.; n<AA; n++ )
    {
        vec2 o = vec2(m,n) / AA - .5;
        vec2 uv = ((g+o)*2.-si)/min(si.x,si.y) * _scale;
        vec2 z = uv, zz;
        vec2 c = _c;
		c.y += sin(iTime) * _limit;
        float it = 0.;
        for (int i=0;i<_niter;i++)
        {
			zz = z;
            z = zinv( _k * zmul(z, z) - c);
			if( shape(z) > _dist ) break;
            it++;
        }

		vec4 sec = _colorVar.x + it * _colorVar.y + vec4(_color,1);
		
		f += .5 + .5 * sin(sec - shape(zz) / shape(z));
    }
    
    f /= AA * AA;
}

void main() {
    fragColor = vec4(0,0,0,1);
    mainImage(fragColor, gl_FragCoord.xy);
    fragColor.a = 1.0;
}
