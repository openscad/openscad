// Empty
difference();
// No children
difference() { }

// Basic
difference() {
  cube([10,10,10], center=true);
  cylinder(r=4, h=20, center=true);
}

// Two negative objects
translate([0,12,0]) difference() {
  cube([10,10,10], center=true);
  cylinder(r=4, h=11, center=true);
  rotate([0,90,0]) cylinder(r=4, h=11, center=true);
}

// Not intersecting
translate([12,12,0]) difference() {
  cube([10,10,10], center=true);
  translate([0,0,7.01]) cylinder(r=4, h=4, center=true);
}

// Barely intersecting
translate([24,0,0]) difference() {
  cube([10,10,10], center=true);
  translate([0,0,6.99]) cylinder(r=4, h=4, center=true);
}

// Subtracting something from nothing
translate([24,12,0]) difference() {
  cube([0,10,10], center=true);
  # cylinder(r=4, h=20, center=true);
}

// Non-geometry (echo) statement as first child should be ignored
translate([24,-12,0]) difference() {
  echo("difference-tests");
  cube([10,10,10], center=true);
  cylinder(r=4, h=20, center=true);
}

// Subtracting 2D from 3D
translate([12,0,0]) difference() {
  cube([10,10,10], center=true);
  circle(r=6);
}
