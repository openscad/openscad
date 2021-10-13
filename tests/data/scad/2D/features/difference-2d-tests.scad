difference() {
  square(10, center=true);
  circle(r=4);
}

translate([12,0]) difference() {
  square(10, center=true);
  translate([2,2]) circle(r=2);
  translate([-2,-2]) circle(r=2);
}

// Subtracting something from nothing
translate([12,12]) difference() {
  square([0,10], center=true);
  # circle(r=4);
}

// Non-geometry (echo) statement as first child should be ignored
translate([0,12]) difference() {
  echo("difference-2d-tests");
  square(10, center=true);
  circle(r=4);
}

// Subtract 3D from 2D
translate([24,0]) difference() {
  square(10, center=true);
  sphere(r=4);
}
