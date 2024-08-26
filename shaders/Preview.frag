#version 110

uniform int rendering_mode;
uniform vec2 resolution;
uniform vec4 color1, color2;
varying vec3 vBC;
varying float shading;
uniform sampler2D depth_buffer;

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
  vec4 color = mix(color2, vec4(color1.rgb * shading, color1.a), edgeFactor());

  // if rendering_mode is zero we fall back to a classical rendering schema using a LEQUAL test
  if(rendering_mode == 0)
  {
    gl_FragColor = color;
    return;
  }

  // otherwise we need to test the depth buffer to select the appropriate fragment
  vec2 screenCoords = gl_FragCoord.xy / resolution;
  float surface_depth = texture2D(depth_buffer, screenCoords).x;

  // nearly-equal z-depth
  if( (abs(surface_depth-gl_FragCoord.z) < 0.0001))
  {
      gl_FragColor = color;
      return;
  }

  // we discard fragment that fails to render.
  discard;
}

