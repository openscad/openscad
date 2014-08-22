// Empty
hull();
// No children
hull() { }

// Hull of hull (forces internal cache to be initialized; this has caused a crash earlier)
translate([25,0,0]) hull() hull3test();

module hull3test() {
  hull() {
    cylinder(r=10, h=1);
    translate([0,0,10]) cube([5,5,5], center=true);
  }
}
hull3test();

translate([50,0,0]) hull() {
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

module hull3null() {
  hull() {
    cube(0);
    sphere(0);
  }
}
hull3null();

