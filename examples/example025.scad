
module example025()
{
	radius = 5;
	side   = 25;
	height = 10;
	
	module circles()
	{
		union()
		{
			translate([radius, radius, 0]) circle (radius);
			translate([radius, side - radius, 0]) circle(radius);
			translate([side - radius, side/2 , 0]) circle(radius);
		}
	}

	linear_extrude(height=height) hull () circles();
	translate ([0,0,height]) circles();
}

example025();
