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


 



void main(void) {
vec3 col= color1.rgb ;
 float rimlight =smoothstep(-1.,1.5,1.- pow(abs(dot(normalize(camnormal.xyz),normal)),2.)); 
 
  col*=  max(shading,( rimlight*(smoothstep(0.,1.,rimlight-.025)*1.25+.025) ) ) ;



  gl_FragColor =  vec4( mix(color2.rgb, col,.9)  ,color1.a) ;
}
