echo(version=version());

// Using echo() in expression context can help with debugging
// recursive functions. See console window for output of the
// examples below.

// Simple example just outputting the function input parameters.
function f1(x, y) = echo("f1: ", x, y) 0.5 * x * x + 4 * y + 1;

r1 = f1(3, 5);

// To output the result, there are multiple possibilities, the
// easiest is to use let() to assign the result to a variable
// (y here) which is used for both echo() output and result.
function f2(x) = let(y = pow(x, 3)) echo("f2: ", y) y;

r2 = f2(4);

// Another option is using a helper function where the argument
// is evaluated first and then passed to the result() helper
// where it's printed using echo() and returned as result.
function result(x) = echo("f3: ", x) x;
function f3(x) = result(x * x - 5);

r3 = f3(5);

// A more complex example is a recursive function. Combining
// the two different ways of printing values before and after
// evaluation it's possible to output the input value x when
// descending into the recursion and the result y collected
// when returning.
function f4(x) = echo("f4: ", x = x)
                 let(y = x == 1 ? 1 : x * f4(x - 1))
                 echo("f4: ", y = y)
                 y;

r4 = f4(5);

// Written in 2018 by Torsten Paul <Torsten.Paul@gmx.de>
//
// To the extent possible under law, the author(s) have dedicated all
// copyright and related and neighboring rights to this software to the
// public domain worldwide. This software is distributed without any
// warranty.
//
// You should have received a copy of the CC0 Public Domain
// Dedication along with this software.
// If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
