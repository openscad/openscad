$fn = 5;

module object() {
  cylinder(r = 1, h = 5);
  rotate(45, [1, 0, 0]) cylinder(r = 1, h = 5);
}

minkowski() {
  difference() {
    cube(20, center = true);
    object();
  }
  cube(.5, center = true);
}
