#version 110

uniform vec4 color1, color2;
uniform float totalHalfEdgeThickness; // total thickness of half-edge (per triangle) including fade, (must be >= fade)
uniform float edgeFadeThickness; // thickness of fade (antialiasing) in screen pixels
varying vec3 vBC;
varying float shading;

vec3 smoothstep3f(vec3 edge0, vec3 edge1, vec3 x) {
  vec3 t;
  t = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
  return t * t * (3.0 - 2.0 * t);
}

float edgeFactor() {
  float th = totalHalfEdgeThickness;
  float fade = edgeFadeThickness;
  vec3 d = fwidth(vBC);
  vec3 a3 = smoothstep((th-fade)*d, th*d, vBC);
  return min(min(a3.x, a3.y), a3.z);
}

void main(void) {
  gl_FragColor = mix(color2, vec4(color1.rgb * shading, color1.a), edgeFactor());
}
