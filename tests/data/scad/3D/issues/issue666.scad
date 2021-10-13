// >2 objects which don't all intersect should be empty
intersection() {
  translate([0,0,0]) cube(5, center=true);
  translate([4,0,0]) cube(5, center=true);
  translate([8,0,0]) cube(5, center=true);
  translate([12,0,0]) cube(5, center=true);
}

// Intersecting with cube(0) should be empty
translate([24,0,0]) intersection() {
  cube(0);
  cube(1);
  cube(5);
  cube(10);
}

// Intersecting with empty union should be empty
translate([36,0,0]) intersection() {
  union() {}
  cube(10, center=true);
}

// Intersecting with empty difference should be empty
translate([48,0,0]) intersection() {
  difference() {}
  cube(10, center=true);
}

// Intersecting with empty intersection should be empty
translate([60,0,0]) intersection() {
  intersection() {}
  cube(10, center=true);
}


// Subtracting from cube(0) should be empty
translate([0,12,0]) difference() {
  cube(0);
  cube(10, center=true);
}

// Subtracting from empty intersection should be empty
translate([0,24,0]) difference() {
  intersection() {
      cube();
      translate([2,0,0]) cube();
  }
  cylinder(r=4, h=10, center=true);
}

// Subtracting from empty intersection should be empty
translate([0,36,0]) difference() {
  intersection() {}
  cube(10, center=true);
}

// Subtracting from empty union should be empty
translate([0,48,0]) difference() {
  union() {}
  cube(10, center=true);
}

// Subtracting from empty difference should be empty
translate([0,60,0]) difference() {
  difference() {}
  cube(10, center=true);
}
