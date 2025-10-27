minkowski() {
  union() {
    cube();
    translate([1,1,1]) cube();
  }
  sphere(0.1, $fn=8);
}
