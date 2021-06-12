// Doing a minkowski sum on an object with
// an internal cavity causes rendering artifacts
// in preview mode (similar to convexity issues)
difference() {
  minkowski(convexity=2) {
    difference() {
      cube(19, center = true);
      cube(11, center = true);
    }
    cube(1, center=true);
  }
  translate([0, 0, 15]) cube(30, center=true);
}
