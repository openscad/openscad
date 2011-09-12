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

// FIXME: We could test $fs, $fa, $fn as well
