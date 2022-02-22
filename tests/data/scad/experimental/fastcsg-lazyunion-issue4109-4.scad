translate([20,20]) {
  cube(5);
  cube();
}

translate([10,0])intersection() {
  cube(5);
  cube();
}

translate([10,10])difference() {
  cube(5);
  cube();
}

translate([10,30])union() {
  cube(5);
  cube();
}
