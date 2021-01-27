module A() {
  cube(1);
  sphere(1);
}

module B() {
  cube([0.1, 1, 1], center=true);
  cube([1, 0.1, 1], center=true);
  translate([0, 1, 0]) A();
}

translate([1, 0, 0]) {
  A();
  translate([0, 0, 1]) B();
}
