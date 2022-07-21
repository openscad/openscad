#version 110

uniform vec4 color1, color2;
varying vec3 vBC;
varying float shading;
varying vec4 screenPosition;

vec3 smoothstep3f(vec3 edge0, vec3 edge1, vec3 x) {
  vec3 t;
  t = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
  return t * t * (3.0 - 2.0 * t);
}

float edgeFactor() {
  const float th = 1.414; // total thickness of half-edge (per triangle) including fade, (must be >= fade)
  const float fade = 1.414; // thickness of fade (antialiasing) in screen pixels
  vec3 d = fwidth(vBC);
  vec3 a3 = smoothstep((th-fade)*d, th*d, vBC);
  return min(min(a3.x, a3.y), a3.z);
}

void main(void) {
  gl_FragColor =  vec4( 
  vec3( 
   ( screenPosition.x+1000.)/1000. ,
   ( screenPosition.y+1000.)/1000. ,
   smoothstep(1.,0.,(pow(max(0.,screenPosition.z),0.95)-1.)/1000.)
   
   )
  
  , color1.a) ;
}
