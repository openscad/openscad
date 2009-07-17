
module cutout()
{
		intersection()
		{
			rotate(90, [1 0 0])
			translate([0 0 -50])
				dxf_linear_extrude(
						file = "example007.dxf",
						layer = "cutout1",
						height = 100,
						convexity = 1);
			
			rotate(90, [0 0 1])
			rotate(90, [1 0 0])
			translate([0 0 -50])
				dxf_linear_extrude(
						file = "example007.dxf",
						layer = "cutout2",
						height = 100,
						convexity = 2);
		}
}

module clip()
{
	difference() {
		dxf_rotate_extrude(
				file = "example007.dxf",
				layer="dorn",
				convexity = 3);
		for (r = [0 90])
			rotate(r, [0 0 1])
				cutout();
	}
}

module cutview()
{
	difference()
	{
		difference()
		{
			translate([0 0 -10])
				clip();

			rotate(20, [0 0 1])
				rotate(-20, [0 1 0])
				translate([18 0 0])
				cube(30, center = true);
		}

		# render(convexity = 5) intersection()
		{
			translate([0 0 -10])
				clip();
		
			rotate(20, [0 0 1])
				rotate(-20, [0 1 0])
				translate([18 0 0])
				cube(30, center = true);
		}
	}
}

cutview();
