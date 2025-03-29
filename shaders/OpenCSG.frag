#version 120
uniform vec4 color;
varying float shading;
void main(void) {
  gl_FragColor = vec4(color.rgb * shading, color.a);
}
