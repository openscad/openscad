translate([-50, -50]) square(40, center = true);
translate([ 50, -50]) circle(20);
translate([  0,  50]) polygon([for (a = [0,120,240]) 30 * [sin(a), cos(a)]]);

square([20, 1], center = true);
square([1, 20], center = true);