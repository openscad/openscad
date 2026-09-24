#version 120

invariant gl_Position;

varying vec4 fragColor;

void main(void) {
  gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

  // Assumes both lights are white diffuse and material specular is zero,
  // matching GLView's fixed-function light/material setup. If that setup
  // ever changes this needs to be updated to match.
  vec3 normal = normalize(gl_NormalMatrix * gl_Normal);
  vec3 lightDir0 = normalize(vec3(gl_LightSource[0].position));
  vec3 lightDir1 = normalize(vec3(gl_LightSource[1].position));
  float diffuse = max(dot(normal, lightDir0), 0.0) + max(dot(normal, lightDir1), 0.0);

  fragColor = vec4(gl_Color.rgb * (gl_LightModel.ambient.rgb + diffuse), gl_Color.a);
}
