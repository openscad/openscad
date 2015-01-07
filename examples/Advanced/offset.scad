// offset.scad - Example for offset() usage in OpenSCAD

$fn = 40;

foot_height = 20;

echo(version=version());

module outline(wall = 1) {
	difference() {
		offset(wall / 2) children();
		offset(-wall / 2) children();
	}
}

// offsetting with a positive value and join_type = "round"
// allows to create rounded corners easily
linear_extrude(height = foot_height, scale = 0.5) {
  offset(10, join_type = "round") {
    square(50, center = true);
  }
}

translate([0, 0, foot_height]) {
	linear_extrude(height = 20) {
		outline(wall = 2) circle(15);
	}
}

%cylinder(r = 14, h = 100);
%translate([0, 0, 100]) sphere(r = 30);



// Written in 2014 by Torsten Paul <Torsten.Paul@gmx.de>
//
// To the extent possible under law, the author(s) have dedicated all
// copyright and related and neighboring rights to this software to the
// public domain worldwide. This software is distributed without any
// warranty.
//
// You should have received a copy of the CC0 Public Domain
// Dedication along with this software.
// If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
