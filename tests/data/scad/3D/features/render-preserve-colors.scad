render() difference() {
  union() {
    color("red") cube(10);
    color("green") translate([20,0,0]) cube(10);
  }
  color("blue") translate([-1,-5,5]) cube([32,10,10]);
}
color("purple") translate([-1,-1,-1]) cube([32,12,2]);
