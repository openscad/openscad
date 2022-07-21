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




void main(void) {
	float elevation=((sqrt(abs(vN.z))*sign(vN.z))/2.)+.5;
	vec3 skycol;
	vec3 c1=vec3(74. ,56. ,0.)/255.; 	vec3 c2=vec3(200. ,230. ,237.)/255.;
	vec3 c3=vec3(167.,174. ,255. )/255.; 	vec3 c4=vec3(77.,94. ,177. )/255.;
	vec3 c12=mix(c1,c2,elevation);	vec3 c23=mix(c2,c3,elevation);
	vec3 c34=mix(c3,c4,elevation);	vec3 c1223=mix(c12,c23,elevation);
	vec3 c2334=mix(c23,c34,elevation);
	skycol=mix(c1223,c2334,elevation);

  gl_FragColor =  vec4( mix(skycol,color1.rgb*shading,.5)  ,color1.a) ;
}
