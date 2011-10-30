
difference()
{
	intersection()
	{
		translate([ -25, -25, -25])
		linear_extrude(file = "example008.dxf",
			layer = "G", height = 50, convexity = 3);
		
		rotate(90, [1, 0, 0])
		translate([ -25, -125, -25])
		linear_extrude(file = "example008.dxf",
			layer = "E", height = 50, convexity = 3);
		
		rotate(90, [0, 1, 0])
		translate([ -125, -125, -25])
		linear_extrude(file = "example008.dxf",
			layer = "B", height = 50, convexity = 3);
	}

	intersection()
	{
		translate([ -125, -25, -25])
		linear_extrude(file = "example008.dxf",
			layer = "X", height = 50, convexity = 1);

		rotate(90, [0, 1, 0])
		translate([ -125, -25, -25])
		linear_extrude(file = "example008.dxf",
			layer = "X", height = 50, convexity = 1);
	}
}