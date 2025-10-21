minkowski() {
  union() {
    cube();
    translate([1,1,0]) cube();
  }
  sphere(0.1, $fn=8);
}
