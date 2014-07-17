// Check import<> and use<> all handle UTF-8 correctly for both
// string usage and comparison.
//
// https://github.com/openscad/openscad/issues/875

include <utf8-tests-inc.scad>
use <utf8-tests-use.scad>

m = "☺ - text - 😀 - more text!";

echo("main: ☺ - ⚀⚁⚂⚃ - 😀 - ");

m1();
m2();

if (m == m1) {
	echo("m1 ok");
} else {
	echo("m1 not ok");
}

if (m == f_m2()) {
	echo("m2 ok");
} else {
	echo("m2 not ok");
}
