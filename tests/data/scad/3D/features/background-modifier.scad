difference() {
  sphere(r=10);
  %cylinder(h=30, r=6, center=true);
}
%if (true) cube([25,6,3], center=true);

%translate([0,-9,0]) difference() {
  color("green") cube([10,4,10], center=true);
  color("red") translate([0,-2,0]) sphere(3);
}

// Test modifiers work with minkowski children
translate([25,0,0]) minkowski() {
      %cube(10, center=true);
      cube(5, center=true);
}
