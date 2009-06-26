
module test001()
{
	function r_from_dia(d) = d / 2;

	module rotcy(rot, r, h) {
		rot(rot) cylinder(r = r, h = h);
	}

	difference() {
		sphere(r = r_from_dia(size));
		rotcy([ 0  0 0], cy_r, cy_h);
		rotcy([90  0 0], cy_r, cy_h);
		rotcy([ 0 90 0], cy_r, cy_h);
	}

	size = 10;
	hole = 2;

	cy_r = r_from_dia(hole);
	cy_h = r_from_dia(size * 1.5);
}

module test002()
{
	difference() {
		cube([2 2 0.5], true);
		cube([0.5 0.5 2], true);
	}
}

module test003()
{
	intersection() {
		difference() {
			union() {
				cube([3 3 3], center = true);
				trans([0 0 -2.5]) cube([1.5 1.5 5], center = true);
			}
			union() {
				cube([5 1 1], center = true);
				cube([1 5 1], center = true);
				cube([1 1 5], center = true);
			}
		}
		trans([0 0 0.5])
			cylinder(h = 5, r1 = 2, r2 = 0.5, center = true);
	}
}

module test004()
{
	intersection() {
		difference() {
			cylinder(h = 5, r1 = 2, r2 = 0.5, center = true);
			cylinder(h = 6, r1 = 0.7, r2 = 0.7, center = true);
		}
		cube(3);
	}
}

module test005()
{
	difference() {
		union() {
			cube([3 3 3], center = true);
			cube([4 1.5 1.5], center = true);
			cube([1.5 4 1.5], center = true);
			cube([1.5 1.5 4], center = true);
		}
		union() {
			cube([5 1 1], center = true);
			cube([1 5 1], center = true);
			cube([1 1 5], center = true);
		}
	}
}

// test005();

difference() {
	cube(8, center = true);
	sphere(5);
}
