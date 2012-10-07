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

// Don't Crash (issue 188)

translate([-5,-5,-5]) {
  hull() {
    intersection() {
      cube([1,1,1]);
      translate([-1,-1,-1]) cube([1,1,1]);
    }
  }
}
