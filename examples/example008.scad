
difference()
{
	intersection()
	{
		translate([ -25, -25, -25])
		linear_extrude(height = 50, convexity = 3)
			import(file = "example008.dxf", layer = "G");
		
		rotate(90, [1, 0, 0])
		translate([ -25, -125, -25])
		linear_extrude(height = 50, convexity = 3)
			import(file = "example008.dxf", layer = "E");
		
		rotate(90, [0, 1, 0])
		translate([ -125, -125, -25])
		linear_extrude(height = 50, convexity = 3)
			import(file = "example008.dxf", layer = "B");
	}

	intersection()
	{
		translate([ -125, -25, -26])
		linear_extrude(height = 52, convexity = 1)
			import(file = "example008.dxf", layer = "X");

		rotate(90, [0, 1, 0])
		translate([ -125, -25, -26])
		linear_extrude(height = 52, convexity = 1)
			import(file = "example008.dxf", layer = "X");
	}
}