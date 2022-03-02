// recursionscad:  Basic recursion example 

// Recursive functions are very powerful for calculating values.
// A good number of algorithms make use of recursive definitions,
// e.g the calculation of the factorial of a number.
// The ternary operator " ? : " is the easiest way to define the
// termination condition.
// Note how the following simple implementation will never terminate
// when called with a negative value. This will produce an error after
// some time when OpenSCAD detects the endless recursive call.
function factorial(n) = n == 0 ? 1 : factorial(n - 1) * n;

color("cyan") text(str("6! = ", factorial(6)), halign = "center");

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
