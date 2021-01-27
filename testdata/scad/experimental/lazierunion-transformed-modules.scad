module A() {
  cube(1);
  cube(2);
}

module B() {
  cube(3);
  cube(4);
  translate([0, 1, 0]) A();
}

translate([1, 0, 0]) {
  A();
  translate([0, 0, 1]) B();
}
