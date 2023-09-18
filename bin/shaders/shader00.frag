#version 330
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec2 vUV;

uniform float iTime;
uniform vec3 iResolution;

// https://www.shadertoy.com/view/4sfczN
// Created by Stephane Cuillerdier - Aiekick/2017 (twitter:@aiekick)

const vec3 ld = vec3(0.,1., .5);

float t = 0.;

float fullAtan(vec2 p)
{
    return step(0.0,-p.x)*3.1415926535 + sign(p.x) * atan(p.x, sign(p.x) * p.y);
}

float fractus(vec2 p, vec2 v)
{
	vec2 z = p;
    vec2 c = v;
	float k = 1., h = 1.0;    
    for (float i=0.;i<30.;i++)
    {
        if (i>7.8) break;
		h *= 4.*k;
		k = dot(z,z);
        if(k > 4.) break;
		z = vec2(z.x * z.x - z.y * z.y, 2. * z.x * z.y) + c;
    }
	return sqrt(k/h)*log(k);   
}

vec2 df(vec3 p)
{
	float a = fullAtan(p.xz)*0.5 +  iTime * 0.5; // axis y
    
    vec2 rev = vec2(length(p.xz),p.y) - 1.52;
    rev *= mat2(cos(a),-sin(a),sin(a),cos(a)); // rot near axis y
	
	vec2 res = vec2(100);
	
	float ftus = fractus(rev, vec2(-0.3,-0.649));
	
	float thickStep = 0.12;
	
	for (int i =0;i <5;i++)
	{
		float lay = max(ftus, -ftus - thickStep * float(i+1)); // change only the interior thickness
		if (lay < res.x)
			res = vec2(lay, float(i));
	}
	
	vec2 q = p.xz;
	float a0 = mix(0.,1.57,res.y/5. * mix(-.1,.1,sin(iTime * 0.5)*.5+.5));
	q *= mat2(cos(a0),-sin(a0),sin(a0),cos(a0));
	
	vec2 cut = vec2(q.y, 10);
	if (cut.x > res.x)
		res = cut;
		
	float plane = p.y+1.;
	if (plane < res.x)
		return vec2(plane,12);
		
	return res;
	
}

vec3 nor( vec3 p, float prec )
{
    vec2 e = vec2( prec, 0. );
    vec3 n = vec3(
		df(p+e.xyy).x - df(p-e.xyy).x,
		df(p+e.yxy).x - df(p-e.yxy).x,
		df(p+e.yyx).x - df(p-e.yyx).x );
    return normalize(n);
}

// from iq code
float softshadow( in vec3 ro, in vec3 rd, in float mint, in float tmax )
{
	float res = 1.0;
    float t = mint;
    for( int i=0; i<1; i++ )
    {
		float h = df( ro + rd*t ).x;
        res = min( res, 8.0*h/t );
        t += h*.25;
        if( h<0.001 || t>tmax ) break;
    }
    return clamp( res, 0., 1. );
}

// from iq code
float calcAO( in vec3 pos, in vec3 nor )
{
	float occ = 0.0;
    float sca = 1.0;
    for( int i=0; i<10; i++ )
    {
        float hr = 0.01 + 0.12*float(i)/4.0;
        vec3 aopos =  nor * hr + pos;
        float dd = df( aopos ).x;
        occ += -(dd-hr)*sca;
        sca *= 0.95;
    }
    return clamp( 1.0 - 3.0*occ, 0.0, 1.0 );    
}

//--------------------------------------------------------------------------
// Grab all sky information for a given ray from camera
// from Dave Hoskins // https://www.shadertoy.com/view/Xsf3zX
vec3 GetSky(in vec3 rd, in vec3 sunDir, in vec3 sunCol)
{
	float sunAmount = max( dot( rd, sunDir), 0.0 );
	float v = pow(1.0-max(rd.y,0.0),6.);
	vec3  sky = mix(vec3(.1, .2, .3), vec3(.32, .32, .32), v);
	sky = sky + sunCol * sunAmount * sunAmount * .25;
	sky = sky + sunCol * min(pow(sunAmount, 800.0)*1.5, .3);
	return clamp(sky, 0.0, 1.0);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	fragColor = vec4(1);
	
	vec2 g = fragCoord.xy;
	vec2 si = iResolution.xy;
	vec2 uv = (2.*g-si)/min(si.x, si.y);
	
	t = 3.14159*0.5;
	
	vec2 camp = vec2(643./958.,356./816.) * 5.;
	
	vec3 rayOrg = vec3(cos(t),sin(camp.y),sin(t)) * camp.x;
	vec3 camUp = vec3(0,1,0);
	vec3 camOrg = vec3(0,1.5,0);
	
	float fov = .5;
	vec3 axisZ = normalize(camOrg - rayOrg);
	vec3 axisX = normalize(cross(camUp, axisZ));
	vec3 axisY = normalize(cross(axisZ, axisX));
	vec3 rayDir = normalize(axisZ + fov * uv.x * axisX + fov * uv.y * axisY);
	
	float s = 1.;
	float d = 0.;
	vec3 p = rayOrg + rayDir * d;
	float dMax = 20.;
	float sMin = 0.0001;
	
	for (float i=0.; i<250.; i++)
	{
		if (abs(s) < d*d*1e-5 || d>dMax) break; // thanks to iq for the simpler form of the condition
		s = df(p).x;
		d += s * 0.3;
		p = rayOrg + rayDir * d;	
	}
	
    vec3 sky = GetSky(rayDir, ld, vec3(1.5));
    
	if (d<dMax)
	{
		vec3 n = nor(p, 0.01178);
		
		// 	iq primitive shader : https://www.shadertoy.com/view/Xds3zN
		float r = mod( floor(5.0*p.z) + floor(5.0*p.x), 2.0);
        fragColor.rgb = 0.4 + 0.1*r*vec3(1.0);

        // iq lighting
		float occ = calcAO( p, n );
        float amb = clamp( 0.5+0.5*n.y, 0.0, 1.0 );
        float dif = clamp( dot( n, ld ), 0.0, 1.0 ) * (df(p+n*1.4).x);
        float spe = pow(clamp( dot( rayDir, ld ), 0.0, 1.0 ),16.0);

        dif *= softshadow( p, ld, 0.1, 10. );

        vec3 brdf = vec3(0.0);
        brdf += 1.20*dif*vec3(1.00,0.90,0.60);
        brdf += 1.20*spe*vec3(1.00,0.90,0.60)*dif;
        brdf += 0.30*amb*vec3(0.50,0.70,1.00)*occ;
        brdf += 0.02;
        fragColor.rgb *= brdf;

        fragColor.rgb = mix( fragColor.rgb, sky, 1.0-exp( -0.01*d*d ) ); 
	}
	else
	{
		fragColor.rgb = sky;
	}
}

void main() {
    fragColor = vec4(0,0,0,1);
    mainImage(fragColor, gl_FragCoord.xy);
    fragColor.a = 1.0;
}
