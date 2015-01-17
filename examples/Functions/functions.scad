echo(version=version());

// Functions can be defined to simplify code using lots of
// calculations.

// Simple example with a single function argument (which should
// be a number) and returning a number calculated based on that.
function f(x) = 0.5 * x + 1;

color("red")
    for (a = [ -100 : 5 : 100 ])
        translate([a, f(a), 0]) cube(2, center = true);

// Functions can call other functions and return complex values
// too. In this case a 3 element vector is returned which can
// be used as point in 3D space or as vector (in the mathematical
// meaning) for translations and other transformations.
function g(x) = [ 5 * x + 20, f(x) * f(x) - 50, 0 ];

color("green")
    for (a = [ -200 : 10 : 200 ])
        translate(g(a / 8)) sphere(1);



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
