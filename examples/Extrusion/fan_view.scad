echo(version=version());

bodywidth = dxf_dim(file = "fan_view.dxf", name = "bodywidth");
fanwidth = dxf_dim(file = "fan_view.dxf", name = "fanwidth");
platewidth = dxf_dim(file = "fan_view.dxf", name = "platewidth");
fan_side_center = dxf_cross(file = "fan_view.dxf", layer = "fan_side_center");
fanrot = dxf_dim(file = "fan_view.dxf", name = "fanrot");

% linear_extrude(height = bodywidth, center = true, convexity = 10)
	import(file = "fan_view.dxf", layer = "body");

% for (z = [+(bodywidth/2 + platewidth/2),
		-(bodywidth/2 + platewidth/2)])
{
	translate([0, 0, z])
	linear_extrude(height = platewidth, center = true, convexity = 10)
		import(file = "fan_view.dxf", layer = "plate");
}

intersection()
{
	linear_extrude(height = fanwidth, center = true, convexity = 10, twist = -fanrot)
		import(file = "fan_view.dxf", layer = "fan_top");
		
	// NB! We have to use the deprecated module here since the "fan_side"
	// layer contains an open polyline, which is not yet supported
	// by the import() module.
	rotate_extrude(convexity = 10)
		import(file = "fan_view.dxf", layer = "fan_side", origin = fan_side_center);
}

// Written by Clifford Wolf <clifford@clifford.at> and Marius
// Kintel <marius@kintel.net>
//
// To the extent possible under law, the author(s) have dedicated all
// copyright and related and neighboring rights to this software to the
// public domain worldwide. This software is distributed without any
// warranty.
//
// You should have received a copy of the CC0 Public Domain
// Dedication along with this software.
// If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
