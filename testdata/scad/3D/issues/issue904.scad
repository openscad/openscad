difference() {
  cylinder($fn=16, h = 4, r = 12.3, center = true);

 translate([4, 12.0, 0]) rotate(a=[0, 0, -90])
   difference() {
     cube(size = [5.5, 5.5, 10], center=true);
     translate([2.75, 2.75, 0]) cylinder($fn = 8, h = 11, r1 = 5.5, r2 = 5.5, center = true);
   }
}
