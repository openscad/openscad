
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

