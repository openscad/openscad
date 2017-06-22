//Test blank
use <>

//Test that the entire path is pushed onto the stack upto the last '/' 
use <sub1/sub2/sub3/sub4/use-test2.scad>

//Test that a non existent path/file doesn't screw things up
use <non/existent/path/non-file>

//Test with empty path 
use <use-test5.scad>

//Test without preceding space
use<use-test5.scad>

//Test that filenames with spaces work
use <use test6.scad>

//Test with empty file
use<test/>

//Test with empty path and file
use </>

//Test with absolute path
use <@CMAKE_SOURCE_DIR@/../testdata/scad/misc/sub2/test7.scad>

// Test simple MCAD library
use <MCAD/teardrop.scad>

// Test MCAD library which includes another file
use <MCAD/math.scad>

// Test MCAD library which uses another file
use <MCAD/servos.scad>

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

  // MCAD
  translate([0,-4,0]) teardrop(0.3, 1.5, 90);
  translate([-2,-4,0]) cube([deg(0.5)/20,0.5,0.5], center=true);
  translate([2,-4,-0.5]) scale(0.05) alignds420([0,0,0], [0,0,0]);
}

test1();
