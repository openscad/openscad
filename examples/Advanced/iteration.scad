echo(version=version());

module example006()
{
	module edgeprofile()
	{
		render(convexity = 2) difference() {
			cube([20, 20, 150], center = true);
			translate([-10, -10, 0])
				cylinder(h = 80, r = 10, center = true);
			translate([-10, -10, +40])
				sphere(r = 10);
			translate([-10, -10, -40])
				sphere(r = 10);
		}
	}

	difference()
	{
		cube(100, center = true);
		for (rot = [ [0, 0, 0], [1, 0, 0], [0, 1, 0] ]) {
			rotate(90, rot)
				for (p = [[+1, +1, 0], [-1, +1, 90], [-1, -1, 180], [+1, -1, 270]]) {
					translate([ p[0]*50, p[1]*50, 0 ])
						rotate(p[2], [0, 0, 1])
							edgeprofile();
				}
		}
		for (i = [
			[ 0, 0, [ [0, 0] ] ],
			[ 90, 0, [ [-20, -20], [+20, +20] ] ],
			[ 180, 0, [ [-20, -25], [-20, 0], [-20, +25], [+20, -25], [+20, 0], [+20, +25] ] ],
			[ 270, 0, [ [0, 0], [-25, -25], [+25, -25], [-25, +25], [+25, +25] ] ],
			[ 0, 90, [ [-25, -25], [0, 0], [+25, +25] ] ],
			[ 0, -90, [ [-25, -25], [+25, -25], [-25, +25], [+25, +25] ] ]
		]) {
			rotate(i[0], [0, 0, 1]) rotate(i[1], [1, 0, 0]) translate([0, -50, 0])
				for (j = i[2])
					translate([j[0], 0, j[1]]) sphere(10);
		}
	}
}

example006();

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
