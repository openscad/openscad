/**
 * Servo outline library
 *
 * Authors:
 *   - Eero 'rambo' af Heurlin 2010-
 *
 * License: LGPL 2.1
 */

use <triangles.scad>

/**
 * Align DS420 digital servo
 *
 * @param vector position The position vector
 * @param vector rotation The rotation vector
 * @param boolean screws If defined then "screws" will be added and when the module is differenced() from something if will have holes for the screws
 * @param number axle_lenght If defined this will draw "backgound" indicator for the main axle
 */
module alignds420(position, rotation, screws = 0, axle_lenght = 0)
{
	translate(position)
	{
		rotate(rotation)
	    {
			union()
			{
				// Main axle
				translate([0,0,17])
				{
					cylinder(r=6, h=8, $fn=30);
					cylinder(r=2.5, h=10.5, $fn=20);
				}
				// Box and ears
				translate([-6,-6,0])
				{
					cube([12, 22.8,19.5], false);
					translate([0,-5, 17])
					{
						cube([12, 7, 2.5]);
					}
					translate([0, 20.8, 17])
					{
						cube([12, 7, 2.5]);
					}
				}
				if (screws > 0)
				{
					translate([0,(-10.2 + 1.8),11.5])
					{
						# cylinder(r=1.8/2, h=6, $fn=6);
					}
					translate([0,(21.0 - 1.8),11.5])
					{
						# cylinder(r=1.8/2, h=6, $fn=6);
					}

				}
				// The large slope
				translate([-6,0,19])
				{
					rotate([90,0,90])
					{
						triangle(4, 18, 12);
					}
				}

				/**
				 * This seems to get too complex fast
				// Small additional axes
				translate([0,6,17])
				{
					cylinder(r=2.5, h=6, $fn=10);
					cylinder(r=1.25, h=8, $fn=10);
				}
				// Small slope
				difference()
				{
					translate([-6,-6,19.0])
					{
						cube([12,6.5,4]);
					}
					translate([7,-7,24.0])
					{
						rotate([-90,0,90])
						{
				            triangle(3, 8, 14);
						}
					}

				}
				*/
				// So we render a cube instead of the small slope on a cube
				translate([-6,-6,19.0])
				{
					cube([12,6.5,4]);
				}
			}
			if (axle_lenght > 0)
			{
				% cylinder(r=0.9, h=axle_lenght, center=true, $fn=8);
			}
		}
	}
}

// Tests:
module test_alignds420(){alignds420(screws=1);}
