#version 110
// https://www.lighthouse3d.com/tutorials/glsl-12-tutorial/
uniform vec4 color1, color2;
varying vec3 vBC;
//uniform vec2 uv;                 // uv coordinates
uniform sampler2D tex;
varying vec3 lightDir,normal;
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

  vec3 ct,cf;
  vec4 texel;
  float intensity,at,af;
  intensity = max(dot(lightDir,normalize(normal)),0.0);
 
  cf = intensity * (gl_FrontMaterial.diffuse).rgb +
                gl_FrontMaterial.ambient.rgb;
  af = gl_FrontMaterial.diffuse.a;
  texel = texture2D(tex,gl_TexCoord[0].st);
 
  ct = texel.rgb;
  at = texel.a;
  gl_FrawColor = texel;
//  gl_FragColor = vec4(ct * cf, at * af);
//  gl_FragColor = mix(color2, vec4(color1.rgb * shading, color1.a), edgeFactor());
}
