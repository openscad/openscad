translate([59.5, -40, 2]) rotate(90) translate([2,31.5,26])
  cube(size = [4, 23, 4], center = false);

translate([5,-35,2]) {
    difference() {
	rotate(90) {
	    translate([-3,-3,1]) cube(size = [3, 6, 30]);
	    rotate_extrude($fn = 14) {
		polygon(points = [[0, 18], [0, 30], [7, 26]]);
	    }
	}
	cylinder(h = 30, r = 1);
    }
}
