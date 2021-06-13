echo(version=version());

// simple 2D -> 3D extrusion of a rectangle
color("red")
    translate([0, -30, 0])
        linear_extrude(height = 20)
            square([20, 10], center = true);

// using the scale parameter a frustum can be constructed
color("green")
    translate([-30, 0, 0])
        linear_extrude(height = 20, scale = 0.2)
            square([20, 10], center = true);

// with twist the extruded shape will rotate around the Z axis
color("cyan")
    translate([30, 0, 0])
        linear_extrude(height = 20, twist = 90)
            square([20, 10], center = true);

// combining both relatively complex shapes can be created
color("gray")
    translate([0, 30, 0])
        linear_extrude(height = 40, twist = -360, scale = 0, center = true, $fs=1, $fa=1)
            square([20, 10], center = true);

// Written in 2015 by Torsten Paul <Torsten.Paul@gmx.de>
//
// To the extent possible under law, the author(s) have dedicated all
// copyright and related and neighboring rights to this software to the
// public domain worldwide. This software is distributed without any
// warranty.
//
// You should have received a copy of the CC0 Public Domain
// Dedication along with this software.
// If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
