// logo.scad - Basic example of difference() usage in OpenSCAD

size = 50;
hole = 25;
cylinderHeight = size * 1.25;
$fn = 32; // Increasing this makes the logo smoother

// One positive object (sphere) and three negative objects (cylinders)
difference() {
    sphere(d=size);

    cylinder(d=hole, h=cylinderHeight, center=true);
    // The '#' operator highlights the object
    #rotate([90, 0, 0]) cylinder(d=hole, h=cylinderHeight, center=true);
    rotate([0, 90, 0]) cylinder(d=hole, h=cylinderHeight, center=true);
}

echo(version=version());
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

