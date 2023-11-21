$fn=8;
sphere(r = 3.75);
rotate([90,0, 180]) {
    translate([0,0,-0.9375]) {
	intersection() {
	    cylinder(h = 14.9375, r1 = 3.63281, r2 = 7.5, center = false);
	    translate([0,0,8.775]) cube([7.5, 7.5, 17.75], center = true);
	}
    }
}
rotate([90,0, 225]) {
    translate([0,0,-0.9375]) {
	intersection() {
	    cylinder(h = 14.9375, r1 = 3.63281, r2 = 7.5, center = false);
	    translate([0,0,8.775]) cube([7.5, 7.5, 17.75], center = true);
	}
    }
}
