translate([54.5, -10, 2]) 
    rotate(90)
	translate([2,30,26]) {
	    difference() {
		cube(size = [4, 21.5, 4]);
		translate([-0.1,-1.1,2]) cube(size = [2.1, 23.5, 2.1]);
	    }
	}

translate([-3,-8,12]) cube(size = [6, 3, 20]);
translate([0, -5, 2]) rotate(90) rotate_extrude($fn = 10) polygon(points = [[0, 18], [0, 30], [7, 26]]);
