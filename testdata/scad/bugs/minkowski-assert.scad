minkowski() {
 cube(20, center=true);
 rotate([20, 30, 40])
   difference() {
     cube(5, center=true);
     cube([1, 1, 10], center=true);
   }
}
