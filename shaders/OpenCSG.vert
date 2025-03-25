#version 120
varying float shading;

void main() {
  vec3 normal, lightDir;
  normal = normalize(gl_NormalMatrix * gl_Normal);
  lightDir = normalize(vec3(gl_LightSource[0].position));
  shading = 0.2 + abs(dot(normal, lightDir));
  gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
