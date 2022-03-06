intersection() {
  union() {
    cube(1);
    translate([0.5, 0.5, 0]) cube(1);
  }
  cube(10, center=1); // Just to ensure fast-union doesn't drop the union above we're trying to test
}
