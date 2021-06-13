//Test nested include
include <include-test3.scad>

//Test relative file location
include <../include-test4.scad>

test2_variable = 0.7;

module test2()
{
  cube(center=true);
}
