
// example016.stl is derived from Mblock.stl
// (c) 2009 Will Langford licensed under
// the Creative Commons - GNU GPL license.
// http://www.thingiverse.com/thing:753
//
// Jonas Pfeil converted the file to binary
// STL and duplicated its content.

module blk1() {
	cube([ 65, 28, 28 ], center = true);
}

module blk2() {
	difference() {
		translate([ 0, 0, 7.5 ])
			cube([ 60, 28, 14 ], center = true);
		cube([ 8, 32, 32 ], center = true);
	}
}

module chop() {
	translate([ -14, 0, 0 ])
		import(file = "example016.stl", convexity = 12);
}

difference() {
	blk1();
	for (alpha = [0, 90, 180, 270]) {
		rotate(alpha, [ 1, 0, 0]) render(convexity = 12)
			difference() {
				blk2();
				chop();
			}
	}
}

