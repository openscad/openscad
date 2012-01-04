//Test nested use
use <use-test3.scad>

//Test relative file location
use <../use-test4.scad>

test2_variable = 0.7;

module test2()
{
  translate([2,0,0]) test3();
  translate([2,-2,0]) test4();
  cube(center=true);
}
