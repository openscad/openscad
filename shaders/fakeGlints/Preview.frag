#version 110
// metallic
uniform vec4 color1, color2;
varying vec3 vBC;
varying float shading;
varying vec4 screenPosition;
varying vec3 normal; 
varying vec3 lightDir; 
varying vec4 camnormal; 
varying vec3 vN;
varying vec3 worldPosition;



//----------------------------------------------------------------------------------------
///  3 out, 3 in...
vec3 hash33(vec3 p3)
{
	p3 = fract(p3 * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yxz+33.33);
    return fract((p3.xxy + p3.yxx)*p3.zyx);

}
vec3 noise33(  vec3 p )
{
    p=p+10000.;
    vec3 i = floor( p );
    vec3 f = fract( p );
	
	vec3 u;
    u.x = f.x*f.x*(3.0-2.0*f.x);
    u.y = f.y*f.y*(3.0-2.0*f.y);
    u.z = f.z*f.z*(3.0-2.0*f.z);

    return mix( 
        mix(
                mix( hash33( i + vec3(0.0,0.0,0.0) ), 
                     hash33( i + vec3(1.0,0.0,0.0) ), u.x),
                mix( hash33( i + vec3(0.0,1.0,0.0) ), 
                     hash33( i + vec3(1.0,1.0,0.0) ), u.x)
               , u.y),
              mix(
                mix( hash33( i + vec3(0.0,0.0,1.0) ), 
                     hash33( i + vec3(1.0,0.0,1.0) ), u.x),
                mix( hash33( i + vec3(0.0,1.0,1.0) ), 
                     hash33( i + vec3(1.0,1.0,1.0) ), u.x)
               , u.y)
        
         , u.z)
        ;
}
vec3 fbm33( vec3 p )
{
   
     mat3 m = mat3(vec3(0.80,  0.60, -0.6).xyz,
                    vec3(0.80,  0.60, -0.6).yzx,
                    vec3(0.80,  0.60, -0.6).zxy);
    vec3 f = vec3(0.0);
    f += 0.5000*noise33( p ); p = m*p*2.02;
    f += 0.2500*noise33( p ); p = m*p*2.03;
    f += 0.1250*noise33( p ); p = m*p*2.01;
    f += 0.0625*noise33( p );
    return f/0.9375;
}

void main(void) {
	vec3 rough=normalize((vec3(-.5)+noise33( worldPosition.xyz*7.5))) ;
   vec3 n= mix(normal,normalize( normal+ (rough *.15) ),(1.-shading*shading));;
 float shadg= 0.2 + abs(pow(dot(n, lightDir),2.));
   vec3 fakeglints=vec3(pow(abs(dot(rough,n)),35.) *shadg*shadg*shading*shading);

  gl_FragColor =  vec4( mix(color2.rgb,fakeglints+color1.rgb*shadg,.9)  ,color1.a) ;
}
