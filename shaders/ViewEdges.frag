#version 120

varying vec4 color;
varying vec3 vBC;
varying float shading;

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
  vec4 color_edge = vec4((color.rgb + vec3(1))/2, 1.0);
  gl_FragColor = mix(color_edge, vec4(color.rgb * shading, color.a), edgeFactor());
}
