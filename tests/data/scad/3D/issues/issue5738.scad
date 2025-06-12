translate([0, 0, 24])
  linear_extrude(height = 6)
    difference() {
     translate([0,20]) square(30, center=true);
     translate([0, 16]) square([6, 10], center=true);
    }

color([0.7, 0.7, 1]) 
translate([3, 32, 0])
  rotate([90, 0, -90])
    linear_extrude(height = 6) 
      polygon([
        [27, 0],
        [27, 23],
        [21, 23],
        [21, 30 -0],
        [11, 30 -0],
        [11, 23],
        [0, 23],
        [0, 0],
      ]);
