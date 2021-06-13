translate([-5,0,0]) hull() {
  union() { 
    rotate([90,0,0]) {
      translate([0,0,0]) {
        cylinder(r=2, h=4, $fn=4, center=true);
        group();
      }
      group();
    }
    translate([0,0,0]) {
      cylinder(r=2, h=4, $fn=4, center=true);
    }
  }
}

hull() {
  intersection() { 
    rotate([90,0,0]) {
      translate([0,0,0]) {
        cylinder(r=2, h=4, $fn=4, center=true);
        group();
      }
      group();
    }
    translate([0,0,0]) {
      cylinder(r=2, h=4, $fn=4, center=true);
    }
  }
}

translate([5,0,0]) hull() {
  difference() { 
    rotate([90,0,0]) {
      translate([0,0,0]) {
        cylinder(r=2, h=4, $fn=4, center=true);
        group();
      }
      group();
    }
    translate([0,0,0]) {
      cylinder(r=2, h=4, $fn=4, center=true);
    }
  }
}
