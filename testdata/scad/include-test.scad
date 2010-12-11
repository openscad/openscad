//Test that the entire path is pushed onto the stack upto the last '/' 
include <sub1/sub2/sub3/sub4/include-test2.scad>

//Test that a non existent path/file doesn't screw things up
include <non/existent/path/non-file>

//Test with empty path 
include <include-test5.scad>

//Test without preceeding space
include<include-test5.scad>

//Test with other strange character that is allowed
include>>>>><include-test5.scad>

//Test that filenames with spaces work
include <include test6.scad>

//Test with empty file
include<test/>

//Test with empty path and file
include </>

//Test with empty
include <>

module test1()
{
	test2();
	test3();
	test4();
	test5();
	test6();

	//Just to give a top level object
	sphere(1);
}

test1();
