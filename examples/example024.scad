// A stack of three circles, biggest first on bottom, smallest last on top.
color([1,0,0]) cylinder(r=10, $rm=INNER_RADIUS);
translate([0,0,1]) color([0,1,0]) cylinder(r=10, $rm=MIDPOINT_RADIUS);
translate([0,0,2]) color([0,0,1]) cylinder(r=10);
// The default behavior is also available as $rm=OUTER_RADIUS

translate([0,0,3]) union() {
	// A yellow cylinder with an INNER_RADIUS hole cut out of it
	color([1,1,0]) difference() {
		cylinder(r=8, h=1);
		cylinder(r=6, h=1.1, $rm=INNER_RADIUS, $fn=30);
	}
	// A cyan cylinder with a default radius where the verticies neatly touch the INNER_RADIUS hole cut above. Deliberately coarse to show difference.
	color([0,1,1]) cylinder(r=6, $fn=13);
}
