minkowski() {
  union() {
    linear_extrude(0.001, scale=0.6666667) {
      circle(r=8);
    }
    translate([0, 0, -0.001]) {
      linear_extrude(0.001, scale=0.75) {
        circle(r=8);
      }
    }
  }
  cube([1,1,1]);
}
