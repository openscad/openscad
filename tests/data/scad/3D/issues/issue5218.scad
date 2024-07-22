difference() {
  cube(10, center=true);
  translate([5,5,5]) minkowski(convexity=2) {
    cube(8, center=true);
    cube(1, center=true);
  }
}