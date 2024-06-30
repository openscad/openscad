difference() {
  sphere(10);
  color("red") translate([0,0,10]) cube(10, center=true);
  translate([0,0,5]) cube(5, center=true);
}