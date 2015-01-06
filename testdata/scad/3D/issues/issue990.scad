hull() {
  for (i = [0:1]) {
    cylinder(h=0);
  }
}

translate([-5, 0, 0]) cube();
translate([5, 0, 0]) cube();
