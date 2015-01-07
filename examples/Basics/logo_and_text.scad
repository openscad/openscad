// logo_and_text.scad - Example for text() usage in OpenSCAD

echo(version=version());

$vpr = [90, 0, 0];
$vpt = [250, 0, 80];
$vpd = 500;

r = 60;
hole = 30;

module t(t, s = 18, style = "") {
	rotate([90, 0, 0])
		linear_extrude(height = 1)
			text(t, size = s, font = str("Liberation Serif", style), $fn = 16);
}

module cut() {
	cylinder(r = hole, h = 2.5 * r, center = true, $fn = 60);
}

module logo() {
	difference() {
		sphere(r = r, $fn = 120);
		cut();
		rotate([0, 90, 0]) cut();
		#rotate([90, 0, 0]) cut();
	}
}

module green() {
	color([81/255, 142/255, 4/255]) children();
}

module black() {
	color([0, 0, 0]) children();
}

translate([110, 0, 80]) {
	translate([0, 0, 30]) rotate([25, 25, -40]) logo();
	translate([100, 0, 40]) green() t("Open", 42, ":style=Bold");
	translate([242, 0, 40]) black() t("SCAD", 42, ":style=Bold");
	translate([100, 0, -10]) black() t("The Programmers");
	translate([160, 0, -40]) black() t("Solid 3D CAD Modeller");
}



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
