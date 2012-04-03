#
# Reported by Triffid Hunter.
# Causes a crash in CCGTermNormalizer::normalizePass()
#

pi = 3.141592653589;
sl = 0.5;

w = 400;

pulley_diam = 28;
bearing_diam = 22; //include size of bearing guides

module crx(size=[1, 1, 1]) {
	linear_extrude(height=size[2])
	square([size[0], size[1]], center=true);
}

module cyl(r=1, h=1, center=false) {
	cylinder(r=r, h=h, center=center, $fn=r * 2 * pi / sl);
}

module lm8uu() {
	cyl(r=15 /2, h=24);
}

module nema17() {
	translate([0, 0, -48]) crx([42, 42, 48]);
	translate([0, 0, -1]) {
		cyl(r=2.5, h=26);
		cyl(r=22.5 / 2, h=3.1);
	}
	translate([0, 0, 2.1]) {
		difference() {
			cyl(r=pulley_diam / 2, h=11.5);
			translate([0, 0, (11.5 - 8) / 2]) rotate_extrude()
				translate([21.5 / 2, 0])
					square(8);
		}
	}
	cyl(r=17.5 / 2, h=19.5);
	for (i=[0:3]) {
		rotate([0, 0, i * 90])
		translate([31 / 2, 31 / 2, -1]) {
			cyl(r=1.5, h=11);
			translate([0, 0, 5.5])
				cyl(r=4, h=30);
		}
	}
}

module bearing608() {
	cyl(r=bearing_diam / 2, h = 7);
}

module rods() {
	for (i=[0:1]) {
		translate([25, 0, i * 70])
			rotate([0, -90, 0])
				cyl(r=4, h=420);
	}
	
	translate([0, 10, -40]) {
		cyl(r=3, h=150);
		translate([0, 0, 34]) cylinder(r=10 / cos(180 / 6) / 2, h=6, $fn=6);
	}
	
	translate([30, 10, -40]) {
		cyl(r=4, h=150);
	}
}

module lm8uu_holder() {
	render()
	difference() {
		union() {
			translate([0, 0, -13]) difference() {
				hull() {
					translate([-9, -9, -1]) cube([18, 1, 28]);
					translate([0, 1, -1]) cyl(r=18 / 2, h = 28);
				}
				translate([-10, 5, -1]) cube([20, 10, 28]);
				translate([-10, -3, 11 - 5]) cube([20, 20, 4]);
				translate([-10, -3, 11 + 5]) cube([20, 20, 4]);
			}
		}
		translate([0, 0, -11]) {
			hull() {
				#lm8uu();
				translate([0, 10, 0]) cyl(r=6, h=24);
			}
			translate([0, 0, -3]) hull() {
				cyl(r=5, h=30);
				translate([0, 10, 0]) cyl(r=5, h=30);
			}
		}
	}
}

module x_end_motor() {
	difference() {
		union() {
			translate([-10, -18, -19]) #cube([50, 2, 90]);
			translate([30, 10, 60]) lm8uu_holder();
			translate([30, 10, -5]) lm8uu_holder();
		}
		#rods();
		
		translate([15.5, -19, 14]) {
			rotate([90, 0, 0]) cyl(r=30 / 2, h= 50, center=true);
			rotate([-90, 45, 0])
				#nema17();
			translate([-w, 4, 0])
				rotate([-90, 0, 0])
					bearing608();
			translate([0, 5, 0]) hull() {
				#translate([0, 0, 24 / 2 - 1.5]) cube([1, 5, 1.5]);
				#translate([-w, 0, bearing_diam / 2 ]) cube([1, 5, 1.5]);
			}
			translate([0, 5, 0]) hull() {
				#translate([0, 0, 24 / -2]) cube([1, 5, 1.5]);
				#translate([-w, 0, bearing_diam / -2 - 1.5]) cube([1, 5, 1.5]);
			}
		}
	}
}

x_end_motor();