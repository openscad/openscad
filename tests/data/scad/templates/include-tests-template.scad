//Test that the entire path is pushed onto the stack up to the last '/' 
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

//Test UTF-8 file name
include <include-☠.scad>

//Test with empty file
include<test/>

//Test with empty path and file
include </>

//Test with absolute path
include <@CMAKE_CURRENT_SOURCE_DIR@/data/scad/misc/sub2/test7.scad>

// Test simple library include
include <testlib/lib-simple.scad>

// Test library include which includes another file
include <testlib/lib-include.scad>

// Test library include which uses another file
include <testlib/lib-use.scad>

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

	// Library tests
	translate([0,-4,0]) cube([lib_var,0.5,0.5], center=true);
	translate([-2,-4,0]) cube([lib_func(1),0.5,0.5], center=true);
	translate([2,-4,0]) lib_use_mod();
}

test1();
