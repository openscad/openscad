// difference_cube.scad - Example for difference() usage in OpenSCAD

echo(version=version());

module difference_cube()
{
	difference() {
		cube(30, center = true);
		sphere(20);
	}
}

difference_cube();



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
