uniform vec3 frag_idcolor;
void main() {
  gl_FragColor = vec4(frag_idcolor, 1.0);
}
