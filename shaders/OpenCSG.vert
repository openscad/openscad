#version 120

invariant gl_Position;

void main() {
  gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
