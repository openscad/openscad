/*
  When exporting this to STL, null polygons appear, causing
  problems normalizing normal vectors (nan output in STL files)
*/

$fs=0.2;

difference() {  
  cube(8);
 
  translate([0,20,4]) rotate([90,0,0]) union() {
    translate([0,-3,14.5]) cube([5.4,6,2.4],center=true);
    translate([0,0,13.3]) rotate([0,0,30]) cylinder(r=3.115,h=2.4,$fn=6);
  }
}

