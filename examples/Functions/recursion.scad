echo(version=version());

// Recursive functions are very powerful for calculating values.
// A good number of algorithms make use of recursive definitions,
// e.g the caluclation of the factorial of a number.
// The ternary operator " ? : " is the easiest way to define the
// termination condition.
// Note how the following simple implementation will never terminate
// when called with a negative value. This will produce an error after
// some time when OpenSCAD detects the endless recursive call.
function factorial(n) = n == 0 ? 1 : factorial(n - 1) * n;

color("cyan")
    translate([0, -30, 0])
        linear_extrude(height = 1)
            text(str("6! = ", factorial(6)), halign = "center");

// With recursive functions very complex results can be generated,
// e.g. calculating the outline of a star shaped polygon. Using
// default parameters can help with simplifying the usage of functions
// that have multiple parameters.
function point(angle) = [ sin(angle), cos(angle) ];
function radius(i, r1, r2) = (i % 2) == 0 ? r1 : r2;
function star(count, r1, r2, i = 0, result = []) = i < count
    ? star(count, r1, r2, i + 1, concat(result, [ radius(i, r1, r2) * point(360 / count * i) ]))
    : result;

color("yellow")
    translate([0, 50, 0])
        linear_extrude(height = 1)
            polygon(star(30, 40, 10));



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
