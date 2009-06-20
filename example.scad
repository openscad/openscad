
difference() {
	sphere(r = 10);
	cylinder(r = 5, h = 15);
	rot([90 0 0]) cylinder(r = 5, h = 15);
	rot([0 90 0]) cylinder(r = 5, h = 15);
}

