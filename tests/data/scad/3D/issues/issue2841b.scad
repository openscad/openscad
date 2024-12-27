minkowski() {
  union() {
    difference() {
      cube([10,10,8], center=true);
      cube([8,8,10], center=true);
    }
    cube([12, 12, 4], center=true);
  }
  sphere(0.5, $fn=8);
}
