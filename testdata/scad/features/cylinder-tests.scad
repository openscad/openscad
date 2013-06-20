cylinder();
translate([1,0,0]) cylinder(r=0);
translate([2,0,0]) cylinder(r1=0, r2=0);
translate([0,-11,0]) cylinder(r=5);
translate([0,11,0]) cylinder(r=5, h=10, center=true);

translate([11,-11,0]) cylinder(h=5, r1=5);
translate([11,0,0]) cylinder(h=5, r1=5, r2=0);
translate([11,11,0]) cylinder(h=8, r1=5, r2=5);

translate([22,-11,0]) cylinder(h=5, r=5, r1=0, center=true);
translate([22,0,0]) cylinder(h=5, r=5, r2=0);
translate([22,11,0]) cylinder(h=15, r=5, r2=5);

// This tests for hexagonal cylinder orientation, since people
// tend to "abuse" this for captured nut slots
translate([-10,0,0]) cylinder(h=2, r=3, $fn=6);

// Test that we clamp number of sections to a minimum of 3
translate([-10, -10, 0]) cylinder(r=3.5356, h=7.0711, $fn=0.1, center=true);
