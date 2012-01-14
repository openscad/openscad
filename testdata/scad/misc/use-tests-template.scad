//Test that the entire path is pushed onto the stack upto the last '/' 
use <sub1/sub2/sub3/sub4/use-test2.scad>

//Test that a non existent path/file doesn't screw things up
use <non/existent/path/non-file>

//Test with empty path 
use <use-test5.scad>

//Test without preceding space
use<use-test5.scad>

//Test with other strange character that is allowed
use>>>>><use-test5.scad>

//Test that filenames with spaces work
use <use test6.scad>

//Test with empty file
use<test/>

//Test with empty path and file
use </>

//Test with absolute path
include <@CMAKE_SOURCE_DIR@/../testdata/scad/misc/sub2/test7.scad>

module test1()
{
  test2();
  // test3() and test4() are not directly included and thus not imported into
  // this scope
  translate([4,0,0]) test3();
  translate([4,-2,0]) test4();
  translate([-2,0,0]) test5();
  translate([-2,-2,0]) test6();
  translate([0,2,0]) test7();

  // test2_variable won't be visible
  translate([0,-2,0]) sphere(test2_variable, $fn=16);
}

test1();
