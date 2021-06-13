// All test items are empty except this one, to give tests something to export.
circle(0.1, $fn=6);

// >2 objects which don't all intersect should be empty
intersection() {
  translate([0,0]) square(5, center=true);
  translate([4,0]) square(5, center=true);
  translate([8,0]) square(5, center=true);
  translate([12,0]) square(5, center=true);
}

// Intersecting with square(0) should be empty
translate([24,0]) intersection() {
  square(0);
  square(1);
  square(5);
  square(10);
}

// Intersecting with empty union should be empty
translate([36,0]) intersection() {
  union() {}
  square(10, center=true);
}

// Intersecting with empty difference should be empty
translate([48,0]) intersection() {
  difference() {}
  square(10, center=true);
}

// Intersecting with empty intersection should be empty
translate([60,0]) intersection() {
  intersection() {}
  square(10, center=true);
}


// Subtracting from square(0) should be empty
translate([0,12]) difference() {
  square(0);
  square(10, center=true);
}

// Subtracting from empty intersection should be empty
translate([0,24]) difference() {
  intersection() {
      square();
      translate([2,0]) square();
  }
  circle(r=4);
}

// Subtracting from empty intersection should be empty
translate([0,36]) difference() {
  intersection() {}
  square(10, center=true);
}

// Subtracting from empty union should be empty
translate([0,48]) difference() { 
  union() {} 
  square(10, center=true); 
}

// Subtracting from empty difference should be empty
translate([0,60]) difference() { 
  difference() {} 
  square(10, center=true); 
}
