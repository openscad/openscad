
module shape1(x, y) {
	translate([50 * x, 50 * y]) difference() {
		square([30, 30], center = true);
		square([8, 8], center = true);
	}
}

module shape2(x, y) {
	translate([50 * x, 50 * y]) {
		polygon(points=[
			[-15, 80],[15, 80],[0,-15],[-8, 60],[8, 60],[0, 5]
		], paths=[
			[0,1,2],[3,4,5]
		]);
	}
}

offset(delta = -1, join_type = "miter") shape2(-1, 2);
shape2(0, 2);
offset(delta = 1, join_type = "miter") shape2(1, 2);

offset(delta = -1, join_type = "miter", miter_limit = 10) shape2(2, 2);
offset(delta = 1, join_type = "miter", miter_limit = 10) shape2(3, 2);

offset(delta = -1, join_type = "bevel") shape2(2, -1);
offset(delta = 1, join_type = "bevel") shape2(3, -1);

offset(delta = -5, join_type = "round") shape1(-1, 1);
shape1(0, 1);
offset(delta = 5, join_type = "round") shape1(1, 1);

offset(-4) shape1(-1, 0);
shape1(0, 0);
offset(4) shape1(1, 0);

offset(delta = -5) shape1(2, 1);
shape1(0, -1);
offset(delta = 5) shape1(1, -1);

// Bug with fragment calculateion with delta < 1 due to abs() instead of std::abs()
translate([-50,-50]) scale([25,25,1])
	offset(delta = 0.9, join_type="round") square(.1);
