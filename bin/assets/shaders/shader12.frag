#version 330
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec2 vUV;

// https://www.shadertoy.com/view/MtVGDt
// Created by Stephane Cuillerdier - @Aiekick/2016

uniform float iTime;
uniform vec3 iResolution;

// count radial section. 
// with this you can have more section. 
// more sections need more id check, but the tech is exactly the same
const float sections = 4.; 	

// some varas
float time = 0.;				// time
float cid = 0., lid = 0.; 	    // current id, last id
mat3 m1;						// matrix used by the pattern merging
mat3 m2;						// matrix used by the pattern merging

// Matrix operations
mat3 getRotXMat(float a){return mat3(1.,0.,0.,0.,cos(a),-sin(a),0.,sin(a),cos(a));}
mat3 getRotYMat(float a){return mat3(cos(a),0.,sin(a),0.,1.,0.,-sin(a),0.,cos(a));}
mat3 getRotZMat(float a){return mat3(cos(a),-sin(a),0.,sin(a),cos(a),0.,0.,0.,1.);}

// tunnel and cam path
vec2 path(float t){return vec2(cos(t*0.08), sin(t*0.08)) * 4.;}

// continuous angle from atan
float cAtan(vec2 uv)
{
	float a = 0.;
	if (uv.x >= 0.) a = atan(uv.x, uv.y);
    if (uv.x < 0.) a = 3.14159 - atan(uv.x, -uv.y);
    return a;
}

// return id of region 
float GetID(vec2 uv) 
{
    return cAtan(uv) * floor(sections) * .5 / 3.14159;
}

// one pettern for each ID
// here id is from range 0 to 1
// so i use Id > section numbers to have continuity
float pattern(vec3 p, mat3 m, float s, float id)
{
	float r = 0.;
	p = abs(fract(p*m*s) - 0.5);
	if (id > 3.) r= max(min(abs(p.x),abs(p.z)),abs(p.y));
    else if (id > 2.) r= max(p.x,abs(p.y)+p.z);
	else if (id > 1.) r= length(p);
    else if (id > 0.) r= max(p.x,-p.y);
	return r;
}

// redirect dispalce func according to ID
// here id is from range 0 to 1
// so i use Id > section numbers to have continuity
float displace(vec3 p, float id)
{
	vec3 a = vec3(0), b = a;
	if (id > 3.) a = vec3(-0.32,0.5,.5), b = vec3(0);
    else if (id > 2.) a = vec3(0.46,0.42,-1.5), b = vec3(0);
    else if (id > 1.) a = vec3(0.36,0.2,-2.28), b = vec3(0.36,0.24,1.62);
	else if (id > 0.) a = vec3(0.62,0.62,-1.02), b = vec3(0);
        
    return 
        (1.-min(pattern(p, m1, a.x, id), pattern(p, m2, a.y, id))) * a.z + 
    	(1.-min(pattern(p, m1, b.x, id), pattern(p, m2, b.y, id))) * b.z;
}

float smin( float a, float b )
{
	float k = 1.;
	float h = clamp( 0.5 + 0.5*(b-a)/k, 0.0, 1.0 );
	return mix( b, a, h ) - k*h*(1.0-h);
}

vec4 map(vec3 p)
{
	p.xy -= path(p.z);													// tunnel path
	
    // mix from displace of last section id with displace of current section id accroding to id range 
    float r = mix(displace(p, lid), displace(p, cid), fract(cid)); 	// id range [0-1]
	
    p *= getRotZMat(p.z*0.05);
	
    p = mod(p, 10.) - 5.;
    
    // tunnel + dispalce
	return vec4(smin(length(p.xz), abs(p.y)) - 1. + r, p);
}

vec3 nor( vec3 pos, float k)
{
	vec3 eps = vec3( k, 0., 0. );
	vec3 nor = vec3(
	    map(pos+eps.xyy).x - map(pos-eps.xyy).x,
	    map(pos+eps.yxy).x - map(pos-eps.yxy).x,
	    map(pos+eps.yyx).x - map(pos-eps.yyx).x );
	return normalize(nor);
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
		s += map(surfPoint).x;
	}
	return 1.-s/(ms*float(iter)); // s < 0. => inside df
}

float SubDensity(vec3 p, float s) 
{
	vec3 n = nor(p,s); 							// precise normale at surf point
	return map(p - n * s).x;						// ratio between df step and constant step
}

// from shane sahders
// Tri-Planar blending function. Based on an old Nvidia writeup:
// GPU Gems 3 - Ryan Geiss: http://http.developer.nvidia.com/GPUGems3/gpugems3_ch01.html
vec3 tex3D( sampler2D tex, in vec3 p, in vec3 n )
{
    n = max((abs(n) - .2)*7., .001);
    n /= (n.x + n.y + n.z );  
	p = (texture(tex, p.yz)*n.x + texture(tex, p.zx)*n.y + texture(tex, p.xy)*n.z).xyz;
    return p*p;
}

// from shane sahders
// Texture bump mapping. Four tri-planar lookups, or 12 texture lookups in total. I tried to 
// make it as concise as possible. Whether that translates to speed, or not, I couldn't say.
vec3 doBumpMap( sampler2D tx, in vec3 p, in vec3 n, float bf)
{
    const vec2 e = vec2(0.001, 0);
    // Three gradient vectors rolled into a matrix, constructed with offset greyscale texture values.    
    mat3 m = mat3( tex3D(tx, p - e.xyy, n), tex3D(tx, p - e.yxy, n), tex3D(tx, p - e.yyx, n));
    vec3 g = vec3(0.299, 0.587, 0.114)*m; // Converting to greyscale.
    g = (g - dot(tex3D(tx,  p , n), vec3(0.299, 0.587, 0.114)) )/e.x; g -= n*dot(n, g);
    return normalize( n + g*bf ); // Bumped normal. "bf" - bump factor.
}

// color arangement is what i choose
// its easy to have another coloration wothout modified the lighting.
// here is an easy way i choose, but you can do alterate more as you want
// here id is from range 0 to 1
// so i use Id > section numbers to have continuity
vec4 params(vec4 f, float id)
{
	vec4 c = f;
    if (id > 3.) c = f.zwyw; 
    else if (id > 2.) c = f.zyzw; 
    else if (id > 1.) c = f.yzyx; 
    else if (id > 0.) c = f.xyzw; 
    return c;
}

vec4 shade(vec3 ro, vec3 rd, float d, vec3 lp)
{
	vec3 p = ro + rd * d;												// surface point
	float sb = SubDensity(p, 0.001, 0.1);							// deep subdensity (10 iterations)
	vec3 bb = blackbody(100.*sb+100.);								// blackbody color according to the subdensity value
	vec3 ld = normalize(lp-p); 											// light dir
	vec3 n = nor(p, .01);												// normal at surface point
	//n = doBumpMap(iChannel0, -p*0.5, n, 0.015);					// use bumpmap fnc of shane here
	vec3 refl = reflect(rd,n);											// reflected ray dir at surf point 
	float amb = 0.08; 													// ambiance factor
	float diff = clamp( dot( n, ld ), 0.0, 1.0 ); 					// diffuse
	float fre = pow( clamp( 1. + dot(n,rd),0.0,1.0), 16. ); 		// fresnel
	float spe = pow(clamp( dot( refl, ld ), 0.0, 1.0 ),25.);		// specular
	float sss = 1. - SubDensity(p*0.1, 5.) * 0.5; 					// one step sub density of df
	return vec4(															// some mix of WTF :) i tried many different things and this i choose :) but maybe there is other things to do
        (diff + fre + bb.x * sss) * amb + diff * 0.5, 
        (diff + fre + bb * sb + sss * 0.3) * amb + spe * 0.6 - diff * sss * 0.05	
    );
}

// get cam 
// g will be gl_FragCoord.xy or uMouse.xy
//  si is screensize
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
    
    mat3 mx = getRotXMat(-7.);
	mat3 my = getRotYMat(-5.);
	mat3 mz = getRotZMat(-3.);
	
	// matrix used by the pattern
    m1 = mx * my * mz;
    m2 = m1 * m1;
	
    time = iTime * 8.;
    
    vec3 cu = vec3(0,1,0);											// Camera Up
  	vec3 cv = vec3(path(time + .1),time + .1); 					// Camera View
    vec3 ro = vec3(path(time),time);								// Camera Origin
	vec3 lp = vec3(path(ro.z + 7.),ro.z + 7.); 					// light point
    vec3 cp = vec3(path(ro.z + 25.2),ro.z + 25.2); 		// center point for Smooth RadClick
	vec3 rd = cam(g, si, ro, cv);									// Camera Direction

	// center point for calculate section id accroding to the tunnel path
	vec2 rdID = rd.xy - normalize(cp-ro).xy;
	
    // fix point in screen center to have progressive id chanage along cam path
	rdID = cam(si*0.5, si, ro, cv).xy - normalize(cp-ro).xy;

	// radial section id
    float id = GetID(rdID); // 0 to 4

    // circular sections id 
    cid = id; 									// current section id
	lid = cid - 1.; 							// last section id
	if (lid < 0.) lid = id + sections - 1.;	// circular 
	
	// Ray Marching
    float s = 1.;
    float d = 0.;
    for(int i=0;i<60;i++)
    {      
        if (log(d*d/s/1e5)>0.) break;
        d += s = map(ro+rd*d).x * .6;
    }
	
	// Color
    f = shade(ro, rd, d, lp);
	
	// mix Color Arangement according to Radial Section Range 
	// mix from params of last section id with params of current section id 
	// according to the range between the two sections with fract(cid) => range 0 to 1
	f = mix(params(f, lid), params(f, cid), fract(cid)); // id range [0-1]
	
	// improve light a little bit
	f = f + f.x*0.3;

	// fog for hide some ray marching artifact cause by the low count iteration (60 here )
    f = mix( f, vec4(0.8), 1.0-exp( -0.001*d*d ) );
        
	// gamma correction for add some contrast without saturation
   	fragColor = sqrt(f*f*f*2.); // gamma correction
}

void main() {
    fragColor = vec4(0,0,0,1);
    mainImage(fragColor, gl_FragCoord.xy);
    fragColor.a = 1.0;
}
