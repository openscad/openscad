/*********************************
* OpenSCAD GridBeam Library      *
* (c) Timothy Schmidt 2013       *
* http://www.github.com/gridbeam *
* License: LGPL 2.1 or later     *
*********************************/

/* Todo:
 - implement "dxf" mode
 - implement hole cutout pattern - interference based on hole size, compatible with two sizes above and below the currently set size.
*/

// zBeam(segments) - create a vertical gridbeam strut 'segments' long
// xBeam(segments) - create a horizontal gridbeam strut along the X axis
// yBeam(segments) - create a horizontal gridbeam strut along the Y axis
// zBolt(segments) - create a bolt 'segments' in length
// xBolt(segments)
// yBolt(segments)
// topShelf(width, depth, corners) - create a shelf suitable for use in gridbeam structures width and depth in 'segments', corners == 1 notches corners
// bottomShelf(width, depth, corners) - like topShelf, but aligns shelf to underside of beams
// backBoard(width, height, corners) - create a backing board suitable for use in gridbeam structures width and height in 'segments', corners == 1 notches corners
// frontBoard(width, height, corners) - like backBoard, but aligns board to front side of beams
// translateBeam([x, y, z]) - translate gridbeam struts or shelves in X, Y, or Z axes in units 'segments'

// To render the DXF file from the command line:
// openscad -x connector.dxf -D'mode="dxf"' connector.scad
mode = "model";
//mode = "dxf";

include <units.scad>

beam_width = inch * 1.5;
beam_hole_diameter = inch * 5/16;
beam_hole_radius = beam_hole_diameter / 2;
beam_is_hollow = 1;
beam_wall_thickness = inch * 1/8;
beam_shelf_thickness = inch * 1/4;

module zBeam(segments) {
if (mode == "model") {
	difference() {
		cube([beam_width, beam_width, beam_width * segments]);
		for(i = [0 : segments - 1]) {
			translate([beam_width / 2, beam_width + 1, beam_width * i + beam_width / 2])
			rotate([90,0,0])
			cylinder(r=beam_hole_radius, h=beam_width + 2);

			translate([-1, beam_width / 2, beam_width * i + beam_width / 2])
			rotate([0,90,0])
			cylinder(r=beam_hole_radius, h=beam_width + 2);
		}
	if (beam_is_hollow == 1) {
		translate([beam_wall_thickness, beam_wall_thickness, -1])
		cube([beam_width - beam_wall_thickness * 2, beam_width - beam_wall_thickness * 2, beam_width * segments + 2]);
	}
}
}

if (mode == "dxf") {

}
}

module xBeam(segments) {
if (mode == "model") {
	translate([0,0,beam_width])
	rotate([0,90,0])
	zBeam(segments);
}

if (mode == "dxf") {

}
}

module yBeam(segments) {
if (mode == "model") {
	translate([0,0,beam_width])
	rotate([-90,0,0])
	zBeam(segments);
}

if (mode == "dxf") {

}
}

module zBolt(segments) {
if (mode == "model") {

}

if (mode == "dxf") {

}
}

module xBolt(segments) {
if (mode == "model") {
}

if (mode == "dxf") {

}
}

module yBolt(segments) {
if (mode == "model") {
}

if (mode == "dxf") {

}
}

module translateBeam(v) {
	for (i = [0 : $children - 1]) {
		translate(v * beam_width) child(i);
	}
}

module topShelf(width, depth, corners) {
if (mode == "model") {
	difference() {
		cube([width * beam_width, depth * beam_width, beam_shelf_thickness]);

		if (corners == 1) {
		translate([-1,  -1,  -1])
		cube([beam_width + 2, beam_width + 2, beam_shelf_thickness + 2]);
		translate([-1, (depth - 1) * beam_width, -1])
		cube([beam_width + 2, beam_width + 2, beam_shelf_thickness + 2]);
		translate([(width - 1) * beam_width, -1, -1])
		cube([beam_width + 2, beam_width + 2, beam_shelf_thickness + 2]);
		translate([(width - 1) * beam_width, (depth - 1) * beam_width, -1])
		cube([beam_width + 2, beam_width + 2, beam_shelf_thickness + 2]);
		}
	}
}

if (mode == "dxf") {

}
}

module bottomShelf(width, depth, corners) {
if (mode == "model") {
	translate([0,0,-beam_shelf_thickness])
	topShelf(width, depth, corners);
}

if (mode == "dxf") {

}
}

module  backBoard(width, height, corners) {
if (mode == "model") {
	translate([beam_width, 0, 0])
	difference() {
		cube([beam_shelf_thickness, width * beam_width, height * beam_width]);

		if (corners == 1) {
		translate([-1,  -1,  -1])
		cube([beam_shelf_thickness + 2, beam_width + 2, beam_width + 2]);
		translate([-1, -1, (height - 1) * beam_width])
		cube([beam_shelf_thickness + 2, beam_width + 2, beam_width + 2]);
		translate([-1, (width - 1) * beam_width, -1])
		cube([beam_shelf_thickness + 2, beam_width + 2, beam_width + 2]);
		translate([-1, (width - 1) * beam_width, (height - 1) * beam_width])
		cube([beam_shelf_thickness + 2, beam_width + 2, beam_width + 2]);
		}
	}
}

if (mode == "dxf") {

}
}

module frontBoard(width, height, corners) {
if (mode == "model") {
	translate([-beam_width - beam_shelf_thickness, 0, 0])
	backBoard(width, height, corners);
}

if (mode == "dxf") {

}
}
