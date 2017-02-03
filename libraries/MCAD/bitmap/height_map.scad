/*
Height Map Example
Tony Buser <tbuser@gmail.com>
http://tonybuser.com
http://creativecommons.org/licenses/by/3.0/

Can also dynamically run this by passing an array on the command line:

/Applications/OpenSCAD.app/Contents/MacOS/OpenSCAD -m make -D bitmap=[2,2,2,0,1,3,2,2,2] -D row_size=3 -s height_map.stl height_map.scad
*/

use <bitmap.scad>

block_size = 5;
height = 5;

row_size = 10; // 10x10 pixels
bitmap = [
	1,1,0,0,1,1,0,0,1,1,
	1,1,1,1,1,1,1,1,1,1,
	0,1,2,2,1,1,2,2,1,0,
	0,1,2,1,1,1,1,2,1,0,
	1,1,1,1,3,3,1,1,1,1,
	1,1,1,1,3,3,1,1,1,1,
	0,1,2,1,1,1,1,2,1,0,
	0,1,2,2,1,1,2,2,1,0,
	1,1,1,1,1,1,1,1,1,1,
	1,1,0,0,1,1,0,0,1,1
];

bitmap(bitmap, block_size, height, row_size);
