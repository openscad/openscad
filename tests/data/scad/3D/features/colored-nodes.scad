color("red") hull() {
  cube(10);
  translate([10,0,0]) cube(10);
}
translate ([24,1,1]) color("green") minkowski() {
    cube(8);
    sphere(1);
  }
translate([41,5,5]) color("blue") resize() {
  intersection() {  
    sphere(r=6);
    cube(10, center=true);
  }
}
