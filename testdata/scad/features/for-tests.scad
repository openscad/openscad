for() cylinder(r=4);
translate([10,0,0]) for(i=3) cylinder(r=i);
for(r=[1:5]) translate([r*10-30,10,0]) cylinder(r=r);
for(r=[5:1]) translate([r*10-30,20,0]) cylinder(r=r);
for(r=[1:2:6]) translate([r*10-30,30,0]) difference() {cylinder(r=r, center=true); cylinder(r=r/2, h=2, center=true);}

// Negative range, negative step
for(r=[5:-1:1]) translate([r*10-60,40,0]) cylinder(r=r);
// Negative range, positive step
for(r=[5:1:1]) translate([r*10-30,40,0]) cylinder(r=r);
// Zero step
for(r=[1:0:5]) translate([r*10+60,40,0]) cylinder(r=r);
// Negative step
for(r=[1:-1:5]) translate([r*10-30,50,0]) cylinder(r=r);
// Illegal step value
for(r=[1:true:5]) translate([r*10-60,50,0]) cylinder(r=r);

// FIXME: Nested for
