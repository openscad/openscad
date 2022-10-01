#version 110

uniform vec4 color1, color2;
varying vec3 vBC;
varying vec4 shading;



void main(void) {
	 
 
   gl_FragColor =  vec4  (shading.xyz   , color1.a );
}
