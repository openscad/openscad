// copy of example001 used for camera tests

module example001()
{
	function r_from_dia(d) = d / 2;

	module rotcy(rot, r, h) {
		rotate(90, rot)
			cylinder(r1 = r / 2, r2 = r, h = h, center = true);
	}

	difference() {
		sphere(r = r_from_dia(size));
		rotcy([0, 0, 0], cy_r, cy_h);
		rotcy([1, 0, 0], cy_r, cy_h, $fn = 4);
		rotcy([0, 1, 0], cy_r, cy_h, $fn = 3);
	}

	size = 50;
	hole = 25;

	cy_r = r_from_dia(hole);
	cy_h = r_from_dia(size * 2.5);
}

example001();

