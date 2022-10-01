#version 110

uniform vec4 color1;        // face color
uniform vec4 color2;        // edge color
attribute vec3 barycentric; // barycentric form of vertex coord
                            // either [1,0,0], [0,1,0] or [0,0,1] under normal circumstances (no edges disabled)
varying vec3 vBC;           // varying barycentric coords
varying float shading;      // multiplied by color1. color2 is without lighting

void main(void) {
  gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
  vBC = barycentric;
  vec3 normal, lightDir;
  normal = normalize(gl_NormalMatrix * gl_Normal);
  lightDir = normalize(vec3(gl_LightSource[0].position));
  shading = 0.2 + abs(dot(normal, lightDir));
}
