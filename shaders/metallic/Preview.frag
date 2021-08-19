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


// cosine based palette, 4 vec3 params
// https://iquilezles.org/www/articles/palettes/palettes.htm
vec3 palette( in float t, in vec3 a, in vec3 b, in vec3 c, in vec3 d )
{
    return a + b*cos( 6.28318*(c*t+d) );
}

void main(void) {
	vec3 col=color1.rgb;
	vec3 skycol,skycolx,skycoly,skycolz ;
	                                   // 0.5, 0.5, 0.5	          	0.5, 0.5, 0.5	       1.0, 1.0, 0.5       	0.80, 0.90, 0.30	
	skycolx= 1. *palette(abs(vN.x) ,vec3(0.5, 0.5, 0.5),vec3(		0.5, 0.5, 0.5	),vec3(1.0, 1.0, 0.5),vec3(	3.00, 6.00, 1.40));
    skycolx=normalize(skycolx)*.5;                            // 0.5, 0.5, 0.5		        0.5, 0.5, 0.5	       1.0, 0.7, 0.4	    0.00, 0.15, 0.20
	skycoly= 1. *palette(vN.y*.5+0.5,vec3(0.5, 0.5, 0.5),vec3(		0.5, 0.5, 0.5	),vec3(1.0, 0.7, 0.4),vec3(	4.00, 8.00, 1.40));
	skycoly=normalize(skycoly)*.75;                            // 0.5, 0.5, 0.5		        0.5, 0.5, 0.5	       1.0, 0.7, 0.4	    0.00, 0.15, 0.20
	skycolz= 1.*palette(vN.z*.6+0.3,vec3(0.5, 0.55, 0.65),vec3(		0.5, 0.5, 0.75	),vec3(1.0, 1.0, 1.0),vec3(	0.00, 0.10, 0.20));
    skycol=mix(skycolx,skycoly,.5);
	skycol=mix(skycol,skycolz,.75);

  gl_FragColor =  vec4( mix(skycol,color1.xyz*shading,.75)  ,color1.a) ;
}
