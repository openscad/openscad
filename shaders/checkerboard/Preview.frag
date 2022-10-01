#version 110

uniform vec4 color1, color2;
varying vec3 vBC;
varying vec4 shading;

 // float divlist[8]=float[8](1.,5.,10.,50,100.,500.,1000.,5000.,10000.);
 float divlist  =1.;


void main(void) {

	vec3  col=color1.xyz;
 
 		col = vec3(floor(mod( floor(mod(shading.x/10.,2.))+ floor(mod(shading.y/10.,2.))+ floor(mod(shading.z/10.,2.)),2.)));
		
 
		
 
		
		
   gl_FragColor =  vec4  (col , color1.a );
}
