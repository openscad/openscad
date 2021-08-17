echo("included.scad");
// Issue #837 - non-existing file causes subsequent include to fail
include <not_exist.scad>
include <included2.scad>
