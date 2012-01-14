//Test that the entire path is pushed onto the stack upto the last '/' 
include <sub1/sub2/sub3/sub4/include-test2.scad>

//Test that a non existent path/file doesn't screw things up
include <non/existent/path/non-file>

//Test with empty path 
include <include-test5.scad>

//Test without preceding space
include<include-test5.scad>

//Test with other strange character that is allowed
include>>>>><include-test5.scad>

//Test that filenames with spaces work
include <include test6.scad>

//Test with empty file
include<test/>

//Test with empty path and file
include </>

//Test with absolute path
include <@CMAKE_SOURCE_DIR@/../testdata/scad/misc/sub2/test7.scad>

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
}

test1();
