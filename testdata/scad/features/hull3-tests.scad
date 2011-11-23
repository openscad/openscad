// Empty
hull();
// No children
hull() { }

hull() {
  cylinder(r=10, h=1);
  translate([0,0,10]) cube([5,5,5], center=true);
}

translate([25,0,0]) hull() {
  translate([0,0,10]) cylinder(r=3);
  difference() {
    cylinder(r=10, h=4, center=true);
    cylinder(r=5, h=5, center=true);
  }
}
