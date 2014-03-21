
module thing()
{
	$fa = 30;
	difference() {
		sphere(r = 25);
		cylinder(h = 62.5, r1 = 12.5, r2 = 6.25, center = true);
		rotate(90, [ 1, 0, 0 ]) cylinder(h = 62.5,
				r1 = 12.5, r2 = 6.25, center = true);
		rotate(90, [ 0, 1, 0 ]) cylinder(h = 62.5,
				r1 = 12.5, r2 = 6.25, center = true);
	}
}

module demo_proj()
{
	linear_extrude(center = true, height = 0.5) projection(cut = false) thing();
	% thing();
}

module demo_cut()
{
	for (i=[-20:5:+20]) {
		 rotate(-30, [ 1, 1, 0 ]) translate([ 0, 0, -i ])
			linear_extrude(center = true, height = 0.5) projection(cut = true)
				translate([ 0, 0, i ]) rotate(+30, [ 1, 1, 0 ]) thing();
	}
	% thing();
}

translate([ -30, 0, 0 ]) demo_proj();
translate([ +30, 0, 0 ]) demo_cut();
