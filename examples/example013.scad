
intersection()
{
	linear_extrude(height = 100, center = true, convexity= 3)
		import(file = "example013.dxf");
	rotate([0, 90, 0])
	linear_extrude(height = 100, center = true, convexity= 3)
		import(file = "example013.dxf");
	rotate([90, 0, 0])
	linear_extrude(height = 100, center = true, convexity= 3)
		import(file = "example013.dxf");
}