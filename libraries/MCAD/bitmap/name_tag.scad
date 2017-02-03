/*
Parametric Name Tag 
Tony Buser <tbuser@gmail.com>
http://tonybuser.com
http://creativecommons.org/licenses/by/3.0/
*/

use <bitmap.scad>

// change chars array and char_count
// OpenSCAD has no string or length methods :(
chars = ["R", "E", "P", "R", "A", "P"];
char_count = 6;


// block size 1 will result in 8mm per letter
block_size = 2;
// height is the Z height of each letter
height = 3;
// Append a hole fo a keyring, necklace etc. ?
key_ring_hole = true;

union() {
	translate(v = [0,-block_size*8*char_count/2+block_size*8/2,3]) {
		8bit_str(chars, char_count, block_size, height);
	}
	translate(v = [0,0,3/2]) {
		color([0,0,1,1]) {
			cube(size = [block_size * 8, block_size * 8 * char_count, 3], center = true);
		}
	}
	if (key_ring_hole == true){
		translate([0, block_size * 8 * (char_count+1)/2, 3/2])	
		difference(){
			cube(size = [block_size * 8, block_size * 8 , 3], center = true);
			cube(size = [block_size * 4, block_size * 4 , 5], center = true);
		}
	}
}
