// >2 objects which don't all intersect should be empty
intersection() {
  translate([0,0,0]) cube(5, center=true);
  translate([4,0,0]) cube(5, center=true);
  translate([8,00,]) cube(5, center=true);
  translate([12,0,0]) cube(5, center=true);
}

// Subtracting something from empty intersection should be empty
translate([0,-12,0]) difference() {
  intersection() {
      cube();
      translate([2,0,0]) cube();
  }
  cylinder(r=4, h=10, center=true);
}
