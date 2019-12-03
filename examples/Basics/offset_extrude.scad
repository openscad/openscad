echo(version=version());

$fn = 60;

// offset to smaller
color("red")
offset_extrude( 20, r = -2)
square([20, 10], center = true);


// offset to bigger (with rounding)
color("blue")
translate([30, 0])
offset_extrude( 20, r = 2)
square([20, 10], center = true);

// offset to bigger (no rounding, no chamfer)
color("green")
translate([60, 0])
offset_extrude( 20, delta = 2)
square([20, 10], center = true);


// offset to bigger (no rounding, chamfer)
color("purple")
translate([0, 30])
offset_extrude( 20, delta = 2, chamfer = true)
square([20, 10], center = true);


// multiple slices to deal with dissapearing features, or smother edges
color("white")
translate([30, 30, 20])
rotate([180, 0])
offset_extrude( 20, r = 2, slices = 10)
difference() {
    square([20, 10], center = true);
    square([4, 4], center = true);
}

// Written in 2019 by Kevin Gravier <kevin@mrkmg.com>
//
// To the extent possible under law, the author(s) have dedicated all
// copyright and related and neighboring rights to this software to the
// public domain worldwide. This software is distributed without any
// warranty.
//
// You should have received a copy of the CC0 Public Domain
// Dedication along with this software.
// If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
