
intersection()
{
	dxf_linear_extrude(file = "example013.dxf",
			height = 100, center = true, convexity= 3);
	rotate([0, 90, 0])
	dxf_linear_extrude(file = "example013.dxf",
			height = 100, center = true, convexity= 3);
	rotate([90, 0, 0])
	dxf_linear_extrude(file = "example013.dxf",
			height = 100, center = true, convexity= 3);
}