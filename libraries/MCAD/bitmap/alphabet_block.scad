/*
Parametric Alphabet Block 
Tony Buser <tbuser@gmail.com>
http://tonybuser.com
http://creativecommons.org/licenses/by/3.0/
*/

use <bitmap.scad>

// change to any letter
letter = "A";

union() {
	difference() {
		cube(size = 20);
		translate(v = [2, 2, 17]) {
			cube(size = [16, 16, 5]);
		}
	}

	translate(v = [10, 10, 15]) {
		8bit_char(letter, 2, 5);
	}
}
