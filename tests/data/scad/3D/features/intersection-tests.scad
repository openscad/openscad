// Empty
intersection();

// No children
intersection() { }

intersection() {
  sphere(r=5);
  translate([0,0,3]) cube([4,4,6], center=true);
}

translate([0,12,0]) intersection() {
  cube([10,10,10], center=true);
  cylinder(r=4, h=12, center=true);
}

translate([12,0,0]) intersection() {
  cube([10,10,10], center=true);
  cylinder(r=4, h=12, center=true);
  rotate([0,90,0]) cylinder(r=4, h=12, center=true);
}

translate([12,12,0]) intersection() {
  cube([10,10,10], center=true);
  translate([0,0,7.01]) cylinder(r=4, h=4, center=true);
}

translate([24,0,0]) intersection() {
  cube([10,10,10], center=true);
  translate([0,0,6.99]) cylinder(r=4, h=4, center=true);
}

translate([-12,0,0]) intersection() {
  cube([10,10,10], center=true);
  translate([0,-10,-10]) cube([10,10,10], center=true);
}

translate([-12,12,0]) intersection() {
  cube([10,10,10], center=true);
  translate([0,-9.99,-9.99]) cube([10,10,10], center=true);
}

// Intersecting something with nothing
translate([0,-12,0]) intersection() {
  cylinder(r=4, h=5, center=true);
  cube(0);
}

// Intersecting something with nothing (issue 996)
translate([0,-12,0]) intersection() {
  cube(4, center=true);
  linear_extrude();
}
translate([0,-16,0]) intersection() {
  cube(4, center=true);
  render();
}
translate([0,-20,0]) intersection() {
  cube(4, center=true);
  minkowski();
}

// Intersecting 2D with 3D
translate([12,-12,0]) intersection() {
  cube([5,5,5], center=true);
  circle(r=2);
}

// Non-geometry (echo) statement as first child should be ignored
translate([24,-12,0]) intersection() {
  echo("difference-tests");
  cube([5,5,5], center=true);
  cylinder(r=2, h=20, center=true);
}
