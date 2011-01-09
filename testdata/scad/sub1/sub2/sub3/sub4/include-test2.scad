//Test nested include
include <include-test3.scad>

//Test relative file location
include <../include-test4.scad>

module test2 ()
{
	echo("included from include-test2.scad");
}
