echo(version=version());

// rotate_extrude() always rotates the 2D shape 360 degrees
// around the Z axis. Note that the 2D shape must be either
// completely on the positive or negative side of the X axis.
color("red")
    rotate_extrude()
        translate([10, 0])
            square(5);

// rotate_extrude() uses the global $fn/$fa/$fs settings, but
// it's possible to give a different value as parameter.
color("cyan")
    translate([40, 0, 0])
        rotate_extrude($fn = 80)
            text("  J");

// Using a shape that touches the X axis is allowed and produces
// 3D objects that don't have a hole in the center.
color("green")
    translate([0, 30, 0])
        rotate_extrude($fn = 80)
            polygon( points=[[0,0],[8,4],[4,8],[4,12],[12,16],[0,20]] );


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
