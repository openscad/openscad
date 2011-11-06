
module shape()
{
	difference()
	{
		translate([ -35, -35 ]) intersection()
		{
			union() {
				difference() {
					square(100, true);
					square(50, true);
				}
				translate([ 50, 50 ])
					square(15, true);
			}
			rotate(45) translate([ 0, -15 ]) square([ 100, 30 ]);
		}
	
		rotate(-45) scale([ 0.7, 1.3 ]) circle(5);
	}
	
	import(file = "example009.dxf", layer = "body",
			convexity = 6, scale=2);
}

// linear_extrude(convexity = 10, center = true)
shape();

