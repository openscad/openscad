module LazyChildrenTest() {
  color("Red") children();
}

LazyChildrenTest() {
  cube(10, center=true);
  translate([11,0,0]) sphere(5);
  translate([22,0,0]) cylinder(r=5, h=10, center=true);
}
