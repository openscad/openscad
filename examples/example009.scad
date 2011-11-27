
bodywidth = dxf_dim(file = "example009.dxf", name = "bodywidth");
fanwidth = dxf_dim(file = "example009.dxf", name = "fanwidth");
platewidth = dxf_dim(file = "example009.dxf", name = "platewidth");
fan_side_center = dxf_cross(file = "example009.dxf", layer = "fan_side_center");
fanrot = dxf_dim(file = "example009.dxf", name = "fanrot");

% linear_extrude(height = bodywidth, center = true, convexity = 10)
	import(file = "example009.dxf", layer = "body");

% for (z = [+(bodywidth/2 + platewidth/2),
		-(bodywidth/2 + platewidth/2)])
{
	translate([0, 0, z])
	linear_extrude(height = platewidth, center = true, convexity = 10)
		import(file = "example009.dxf", layer = "plate");
}

intersection()
{
	linear_extrude(height = fanwidth, center = true, convexity = 10, twist = -fanrot)
		import(file = "example009.dxf", layer = "fan_top");
		
	// NB! We have to use the deprecated module here since the "fan_side"
	// layer contains an open polyline, which is not yet supported
	// by the import() module.
	rotate_extrude(file = "example009.dxf", layer = "fan_side",
		origin = fan_side_center, convexity = 10);
}

