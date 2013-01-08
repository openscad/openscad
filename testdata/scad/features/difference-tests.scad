// Empty
difference();
// No children
difference() { }

difference() {
  cube([10,10,10], center=true);
  cylinder(r=4, h=20, center=true);
}

translate([12,0,0]) difference() {
  cube([10,10,10], center=true);
  cylinder(r=4, h=10.5, center=true);
}

translate([0,12,0]) difference() {
  cube([10,10,10], center=true);
  cylinder(r=4, h=11, center=true);
  rotate([0,90,0]) cylinder(r=4, h=11, center=true);
}

translate([12,12,0]) difference() {
  cube([10,10,10], center=true);
  translate([0,0,7.01]) cylinder(r=4, h=4, center=true);
}

translate([24,0,0]) difference() {
  cube([10,10,10], center=true);
  translate([0,0,6.99]) cylinder(r=4, h=4, center=true);
}

// Subtracting something from nothing
translate([24,12,0]) difference() {
  cube([0,10,10], center=true);
  # cylinder(r=4, h=20, center=true);
}
