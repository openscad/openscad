// Minkowski Example
// A small storage tray with rounded edges.
//
// minkowski() traces the second shape over the surface of the first,
// so a box combined with a sphere becomes a box with rounded edges.
// The box is shrunk by the radius first so the final size stays exact.

$fa = 6;
$fs = 0.5;

size = [60, 40, 20]; // outside size of the tray
radius = 4;          // how round the outside edges are
wall = 2;            // wall and floor thickness

module rounded_box(size, radius) {
    minkowski() {
        cube(size - 2 * [radius, radius, radius], center = true);
        sphere(r = radius);
    }
}

// Sit the tray on the build plate
translate([0, 0, size.z / 2])
difference() {
    rounded_box(size, radius);

    // Hollow it out. The inner radius is smaller by the wall thickness,
    // which keeps the walls the same thickness around the corners.
    translate([0, 0, wall])
        rounded_box([size.x - 2 * wall, size.y - 2 * wall, size.z], radius - wall);

    // Cut the rounded top off to leave a flat rim
    translate([0, 0, size.z - radius])
        cube([size.x + 1, size.y + 1, size.z], center = true);
}


// Written by Arijit Aich, 2026

// To the extent possible under law, the author(s) have dedicated all
// copyright and related and neighboring rights to this software to the
// public domain worldwide. This software is distributed without any
// warranty.
//
// You should have received a copy of the CC0 Public Domain
// Dedication along with this software.
// If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
