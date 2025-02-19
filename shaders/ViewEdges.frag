#version 120

uniform float textureFactor;
varying vec4 color;
varying vec3 vBC;
uniform sampler2D tex1;
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
  vec4 texel = texture2D(tex1,gl_TexCoord[0].st);
  vec4 gray; gray.r=0.5; gray.g=0.5; gray.b=0.5; gray.a=1.0; 
  vec4 color_edge = vec4((color.rgb + vec3(1))/2, 1.0);
  gl_FragColor = mix(color_edge, vec4(mix(color.rgb * shading, color.rgb * shading + texel.rgb - gray.rgb, textureFactor), color.a), edgeFactor());
}
