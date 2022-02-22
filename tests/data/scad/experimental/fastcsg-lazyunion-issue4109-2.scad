module base() {
  linear_extrude(5) offset(3) square([20, 8]);
}

module lid() {
  difference() {
    base();
    cube();
  }
}

base();

translate([0, 0, 18]) mirror([0, 0, 1]) lid();
