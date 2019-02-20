//Test that the entire path is pushed onto the stack upto the last '/' 
include <sub1/sub2/sub3/sub4/include-test2.scad>

//Subdir
include <sub1/included.scad>

//Test that a non existent path/file doesn't screw things up
include <non/existent/path/non-file>

//Test with empty path 
include <include-test5.scad>

//Test without preceding space
include<include-test5.scad>

//Test that filenames with spaces work
include <include test6.scad>

//Test with empty file
include<test/>

//Test with empty path and file
include </>

//Test with absolute path
include <@CMAKE_SOURCE_DIR@/../testdata/scad/misc/sub2/test7.scad>

// Test simple MCAD include
include <MCAD/constants.scad>

// Test MCAD include which includes another file
include <MCAD/math.scad>

// Test MCAD include which uses another file
include <MCAD/servos.scad>

module test1()
{
	test2();
	translate([2,0,0]) test3();
	translate([2,-2,0]) test4();
	translate([-2,0,0]) test5();
	translate([-2,-2,0]) test6();
	translate([0,2,0]) test7();

	//Just to give a top level object
	translate([0,-2,0]) sphere(test2_variable, $fn=16);

 	// MCAD
	translate([0,-4,0]) cube([TAU/4,0.5,0.5], center=true);
	translate([-2,-4,0]) cube([deg(0.5)/20,0.5,0.5], center=true);
	translate([2,-4,-0.5]) scale(0.05) alignds420([0,0,0], [0,0,0]);
}

test1();
