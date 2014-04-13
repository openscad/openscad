cube();
cube([1,1,0]); cube([1,0,1]); cube([0,1,1]); cube([0,0,0]);
translate([2,0,0]) cube([2,3,1]);
translate([6,0,0]) cube([2,4,2], center=true);
translate([0,6,0]) cube([2,4,2], center=[true, true, false]);
