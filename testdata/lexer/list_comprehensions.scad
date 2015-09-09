// list_comprehensions.scad - Examples of list comprehension usage

// Basic list comprehension:
// Returns a 2D vertex per iteration of the for loop
// Note: subsequent assignments inside the for loop is allowed
module ngon(num, r) {
  polygon([for (i=[0:num-1], a=i*360/num) [ r*cos(a), r*sin(a) ]]);
}

ngon(3, 10);
translate([20,0]) ngon(6, 8);
translate([36,0]) ngon(10, 6);

// More complex list comprehension:
// Similar to ngon(), but uses an inner function to calculate
// the vertices. the let() keyword allows assignment of temporary variables.
module rounded_ngon(num, r, rounding = 0) {
  function v(a) = let (d = 360/num, v = floor((a+d/2)/d)*d) (r-rounding) * [cos(v), sin(v)];
  polygon([for (a=[0:360-1]) v(a) + rounding*[cos(a),sin(a)]]);
}

translate([0,22]) rounded_ngon(3, 10, 5);
translate([20,22]) rounded_ngon(6, 8, 4);
translate([36,22]) rounded_ngon(10, 6, 3);

// Gear/star generator
// Uses a list comprehension taking a list of radii to generate a star shape
module star(num, radii) {
  function r(a) = (floor(a / 10) % 2) ? 10 : 8;
  polygon([for (i=[0:num-1], a=i*360/num, r=radii[i%len(radii)]) [ r*cos(a), r*sin(a) ]]);
}

translate([0,44]) star(20, [6,10]);
translate([20,44]) star(40, [6,8,8,6]);
translate([36,44]) star(30, [3,4,5,6,5,4]);

echo(version=version());
// Written by Marius Kintel <marius@kintel.net>
//
// To the extent possible under law, the author(s) have dedicated all
// copyright and related and neighboring rights to this software to the
// public domain worldwide. This software is distributed without any
// warranty.
//
// You should have received a copy of the CC0 Public Domain
// Dedication along with this software.
// If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
