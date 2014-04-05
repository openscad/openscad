
// example012.stl is Mblock.stl, (c) 2009 Will Langford
// licensed under the Creative Commons - GNU GPL license.
// http://www.thingiverse.com/thing:753

difference()
{
	sphere(20);
	
	translate([ -2.92, 0.5, +20 ]) rotate([180, 0, 180])
			import("example012.stl", convexity = 5);
}
