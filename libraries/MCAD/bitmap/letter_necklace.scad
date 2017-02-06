/*
Parametric letters for for a necklace
Elmo Mäntynen <elmo.mantynen@iki.fi>
LGPL 2.1
*/

use <bitmap.scad>

// change chars array and char_count
// OpenSCAD has no string or length methods :(
chars = ["M","a","k","e","r","B","o","t"];
char_count = 8;

// block size 1 will result in 8mm per letter
block_size = 2;
// height is the Z height of each letter
height = 3;

//Hole for the necklace
hole_diameter = 5;

module 8bit_str(chars, char_count, block_size, height) {
	echo(str("Total Width: ", block_size * 8 * char_count, "mm"));
	union() {
		for (count = [0:char_count-1]) {
			translate(v = [0, count * block_size * 8, 0]) {
				8bit_char(chars[count], block_size, height);
			}
		}
	}
}

module letter(char, block_size, height, hole_diameter) {
	union() {
		translate(v = [0,0, hole_diameter*1.3]) {
			8bit_char(char, block_size, height);
		}
		translate(v = [0,0,(hole_diameter*1.3)/2]) {
			color([0,0,1,1]) {
				difference() {
					cube(size = [block_size * 8, block_size * 8, hole_diameter+2], center = true);
					rotate([90, 0, 0]) cylinder(h = block_size * 8 + 1, r = hole_diameter/2, center = true);
				}
			}
		}
	}
}

matrix = [["O",  "L", "E", "N", "S"], 
		[ "Y", "OE", "N", "Y", "T"]];

union() {
	for (column = [0:1]) {
		for (row = [0:4]) {
			translate(v=[column*(block_size*1.1)*8, row*(block_size*1.1)*8, 0])
				letter(matrix[column][row], block_size, height, hole_diameter);
		}
	}	
}
