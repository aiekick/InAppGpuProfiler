#version 330
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec2 vUV;

uniform float iTime;
uniform vec3 iResolution;

// https://www.shadertoy.com/view/Xl3SzH
// Created by Stephane Cuillerdier - Aiekick/2016

mat3 RotX(float a){return mat3(1.,0.,0.,0.,cos(a),-sin(a),0.,sin(a),cos(a));}
mat3 RotY(float a){return mat3(cos(a),0.,sin(a),0.,1.,0.,-sin(a),0.,cos(a));}
mat3 RotZ(float a){return mat3(cos(a),-sin(a),0.,sin(a),cos(a),0.,0.,0.,1.);}

const float mPi = 3.14159;
const float m2Pi = 6.28318;
float time = 0.;
float time2 = 0.;

float shape(vec3 p)
{
    return length(p);
}

vec2 df(vec3 p) // from MegaWave 2 (https://www.shadertoy.com/view/ltjXWR)
{
	vec2 res = vec2(1000.);
	
    p *= RotY(-3.14159/4.);
    p *= RotZ(p.z * 0.05);
    
	vec3 q;
	
	// mat 2
	q.x = cos(p.x);
	q.y = p.y * 5. - 25. + 10. * cos(p.x / 7. + time2) + 10. * sin(p.z / 7. + time2);
	q.z = cos(p.z);
    float sphere = shape(q) - 1.;
	if (sphere < res.x)
		res = vec2(sphere, 2.);
	
	// mat 3
	q.x = cos(p.x);
	q.y = p.y * 5. + 25. + 10. * cos(p.x / 7. + time2 + mPi) + 10. * sin(p.z / 7. + time2 + mPi);
	q.z = cos(p.z);
	sphere = shape(q) - 1.;
	if (sphere < res.x)
		res = vec2(sphere, 3.);

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
    for( int i=0; i<30; i++ )
    {
		float h = df( ro + rd*t ).x;
        res = min( res, 8.0*h/t );
        t += clamp( h, 0.02, 0.10 );
        if( h<0.001 || t>tmax ) break;
    }
    return clamp( res, 0.0, 1.0 );
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


// return color from temperature 
//http://www.physics.sfasu.edu/astro/color/blackbody.html
//http://www.vendian.org/mncharity/dir3/blackbody/
//http://www.vendian.org/mncharity/dir3/blackbody/UnstableURLs/bbr_color.html
vec3 blackbody(float Temp)
{
	vec3 col = vec3(255.);
    col.x = 56100000. * pow(Temp,(-3. / 2.)) + 148.;
   	col.y = 100.04 * log(Temp) - 623.6;
   	if (Temp > 6500.) col.y = 35200000. * pow(Temp,(-3. / 2.)) + 184.;
   	col.z = 194.18 * log(Temp) - 1448.6;
   	col = clamp(col, 0., 255.)/255.;
    if (Temp < 1000.) col *= Temp/1000.;
   	return col;
}

// get density of the df at surfPoint
// ratio between constant step and df value
float SubDensity(vec3 surfPoint, float prec, float ms) 
{
	vec3 n;
	float s = 0.;
    const int iter = 8;
	for (int i=0;i<iter;i++)
	{
		n = nor(surfPoint,prec); 
		surfPoint = surfPoint - n * ms; 
		s += df(surfPoint).x;
	}
	return 1.-s/(ms*float(iter)); // s < 0. => inside df
}

float SubDensity(vec3 p, float s) 
{
	vec3 n = nor(p,s); 							// precise normale at surf point
	return df(p - n * s).x;						// ratio between df step and constant step
}

vec4 shade(vec3 ro, vec3 rd, float d, vec3 lp)
{
	vec3 p = ro + rd * d;											// surface point
	float sb = SubDensity(p, 0.001, 0.1);							// deep subdensity (10 iterations)
	vec3 bb = blackbody(100.*sb+100.);								// blackbody color according to the subdensity value
	vec3 ld = normalize(lp-p); 										// light dir
	vec3 n = nor(p, .01);											// normal at surface point
	vec3 refl = reflect(rd,n);										// reflected ray dir at surf point 
	float amb = 0.08; 												// ambiance factor
	float diff = clamp( dot( n, ld ), 0.0, 1.0 ); 					// diffuse
    diff *= softshadow( p, ld, 0.02, 50.);							// soft shadow
	float fre = pow( clamp( 1. + dot(n,rd),0.0,1.0), 16. ); 		// fresnel
	float spe = pow(clamp( dot( refl, ld ), 0.0, 1.0 ),25.);		// specular
	float sss = 1. - SubDensity(p*0.1, 5.) * 0.5; 					// one step sub density of df
	return vec4(													// some mix of WTF :) i tried many different things and this i choose :) but maybe there is other things to do
        (diff + fre + bb.x * sss) * amb + diff * 0.5, 
        (diff + fre + bb * sb + sss * 0.3) * amb + spe * 0.6 - diff * sss * 0.05	
    );
}

// get cam
vec3 cam(vec2 g, vec2 si, vec3 ro, vec3 cv)
{
	vec2 uv = (g+g-si)/si.y;
	vec3 cu = normalize(vec3(0,1,0));
  	vec3 z = normalize(cv-ro);
    vec3 x = normalize(cross(cu,z));
  	vec3 y= cross(z,x);
  	return normalize(z + uv.x*x + uv.y*y);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec4 f = vec4(0);
    
    vec2 g = fragCoord.xy;
    vec2 si = iResolution.xy;
    
    time = iTime * 2.;
    time2 = iTime * 1.;
    
    vec3 cu = vec3(0,1,0);
  	vec3 cv = vec3(time + .1,0,time + .1);
    vec3 ro = vec3(time,0,time);
	
	vec3 lp = ro + vec3(101,10,-5);
   	vec3 rd = cam(g, si, ro, cv);

    float s = 1.;
    float d = 0.;
    for(int i=0;i<250;i++)
    {      
        if (log(d*d/s/1e5)>0.||d>100.)break;
        d += s = df(ro+rd*d).x * .1;
    }
	
    vec3 sky = GetSky(rd, normalize(lp-ro), vec3(2.));

    if (d < 100.)
    {
        f = shade(ro, rd, d, lp);
        f = f.zyzw;
        f = f + f.x*0.3;
        f = mix( f, sky.rbgg, 1.0-exp( -0.001*d*d ) );
    }
    else
    {
        f = sky.rbgg;
    }
        
   	fragColor = sqrt(f*f*f*5.); // gamma correction
}

void main() {
    fragColor = vec4(0,0,0,1);
    mainImage(fragColor, gl_FragCoord.xy);
    fragColor.a = 1.0;
}
