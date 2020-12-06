// animation.scad - Demo of animation usage

// The animation functionality is based simply on a variable $t
// that is changed automatically by OpenSCAD while repeatedly
// showing the model.
// To activate animation, select "View->Animate" from the
// menu; this will cause three fields to appear
// underneath the Preview console: Time, FPS & Steps.
// To commence animation, enter values into the FPS and Steps input
// fields (e.g. 5 FPS and 200 Steps for this animation).
// This is not intended to directly produce real-time animations
// but the image sequence can be exported to generate videos of
// the animation.

// Length of the 2 arm segments, change to see the effects on
// the arm movements.

//length of the red arm
arm1_length = 70;
//length of the green arm
arm2_length = 50;

r = 2;
$fn = 30;

plate();
pos = position($t);
arm(pos[0], pos[1], arm1_length, arm2_length);

// Function describing the X/Y position that should be traced
// by the arm over time.
// The $t variable will be used as parameter for this function
// so the range for t is [0..1].
function position(t) = t < 0.5
    ? [ 200 * t - 50, 30 * sin(5 * 360 * t) + 60 ]
    : [ 50 * cos(360 * (t - 0.5)), 100 * -sin(360 * (t- 0.5)) + 60 ];

// Inverse kinematics functions for a scara style arm
// See http://forums.reprap.org/read.php?185,283327
function sq(x, y) = x * x + y * y;
function angB(x, y, l1, l2) = 180 - acos((l2 * l2 + l1 * l1 - sq(x, y)) / (2 * l1 * l2));
function ang2(x, y, l1, l2) = 90 - acos((l2 * l2 - l1 * l1 + sq(x, y)) / (2 * l2 * sqrt(sq(x, y)))) - atan2(x, y);
function ang1(x, y, l1, l2) = ang2(x, y, l1, l2) + angB(x, y, l1, l2);

// Draw an arm segment with the given color and length.
module segment(col, l) {
    color(col) {
        hull() {
            sphere(r);
            translate([l, 0, 0]) sphere(r);
        }
    }
}

// Draw the whole 2 segmented arm trying to reach position x/y.
// Parameters l1 and l2 are the length of the two arm segments.
module arm(x, y, l1, l2) {
    a1 = ang1(x, y, l1, l2);
    a2 = ang2(x, y, l1, l2);
    sphere(r = 2 * r);
    cylinder(r = 2, h = 6 * r, center = true);
    rotate([0, 0, a1]) segment("red", l1);
    translate(l1 * [cos(a1), sin(a1), 0]) {
        sphere(r = 2 * r);
        rotate([0, 0, a2]) segment("green", l2);
    }
    translate([x, y, -r/2])
        cylinder(r1 = 0, r2 = r, h = 4 * r, center = true);
}

module curve() polygon([for (a = [ 0 : 0.004 : 1]) position(a)]);

// Draws the plate and the traced function using small black cubes.
module plate() {
    %translate([0, 0, -3*r]) {
        translate([0,25,0]) cube([150, 150, 0.1], center = true);
        color("Black") linear_extrude(0.1) difference() {
            curve();
            offset(-1) curve();
        }
    }
}

echo(version=version());
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
