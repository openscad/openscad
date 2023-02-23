#version 110
// https://www.lighthouse3d.com/tutorials/glsl-12-tutorial/
uniform vec4 color1, color2;
uniform int drawEdges;
uniform int textureInd;
varying vec3 vBC;
uniform sampler2D tex;
varying float shading;
varying vec4 drawColor;

vec3 smoothstep3f(vec3 edge0, vec3 edge1, vec3 x) {
  vec3 t;
  t = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
  return t * t * (3.0 - 2.0 * t);
}

float edgeFactor() {
  if(drawEdges != 1) {
    return 1.0; // only mix in the edge color when edges are shown
  }
  const float th = 1.414; // total thickness of half-edge (per triangle) including fade, (must be >= fade)
  const float fade = 1.414; // thickness of fade (antialiasing) in screen pixels
  vec3 d = fwidth(vBC);
  vec3 a3 = smoothstep((th-fade)*d, th*d, vBC);
  return min(min(a3.x, a3.y), a3.z);
}

void main(void) {
  vec4 facecolor = drawColor;

  if(textureInd > 0 ) {
    vec4 texel = texture2D(tex,gl_TexCoord[0].st);
    vec4 gray; gray.r=0.5; gray.g=0.5; gray.b=0.5;
    facecolor.rgb = facecolor.rgb + texel.rgb - gray.rgb;
  } 
  gl_FragColor = mix(color2, vec4(facecolor.rgb * shading, facecolor.a), edgeFactor());
}
