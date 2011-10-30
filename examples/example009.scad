
bodywidth = dxf_dim(file = "example009.dxf", name = "bodywidth");
fanwidth = dxf_dim(file = "example009.dxf", name = "fanwidth");
platewidth = dxf_dim(file = "example009.dxf", name = "platewidth");
fan_side_center = dxf_cross(file = "example009.dxf",
		layer = "fan_side_center");
fanrot = dxf_dim(file = "example009.dxf", name = "fanrot");

% linear_extrude(file = "example009.dxf", layer = "body",
	height = bodywidth, center = true, convexity = 10);

% for (z = [+(bodywidth/2 + platewidth/2),
		-(bodywidth/2 + platewidth/2)])
{
	translate([0, 0, z])
	linear_extrude(file = "example009.dxf", layer = "plate",
		height = platewidth, center = true, convexity = 10);
}

intersection()
{
	linear_extrude(file = "example009.dxf", layer = "fan_top",
		height = fanwidth, center = true, convexity = 10,
		twist = -fanrot);
	rotate_extrude(file = "example009.dxf", layer = "fan_side",
		origin = fan_side_center, convexity = 10);
}

