// Empty
for();
// No children
for(i=2) { }
// Modifier and no children
%for(i=2) { }
#for(i=2) { }

// Null
translate([-10,0,0]) for() cylinder(r=4);

// Scalar
translate([10,0,0]) for(i=3) cylinder(r=i);

// Range
for(r=[1:5]) translate([r*10-30,10,0]) cylinder(r=r);

// Reverse
for(r=[5:1]) translate([r*10-30,20,0]) cylinder(r=r);

// Step
for(r=[1:2:6]) translate([r*10-30,30,0]) difference() {cylinder(r=r, center=true); cylinder(r=r/2, h=2, center=true);}

// Fractional step
for(r=[1.5:0.2:2.5]) translate([r*10-30,30,0]) cube([1, 4*r, 2], center=true);

// Negative range, negative step
for(r=[5:-1:1]) translate([r*10-30,50,0]) cylinder(r=r);

// Negative range, positive step (using backward compatible auto swap of begin and end)
for(r=[5:1]) translate([r*10-30,40,0]) cylinder(r=r);

// Zero step

for(r=[1:0:5]) translate([r*10+60,40,0]) cylinder(r=r);

// Negative step
for(r=[1:-1:5]) translate([r*10-30,50,0]) cylinder(r=r);

// Illegal step value
for(r=[1:true:5]) translate([r*10-60,50,0]) cylinder(r=r);

// Vector
for(r=[1,2,5]) translate([r*10-30,0,0]) cylinder(r=r);

nan = 0/0;
inf = 1/0;
ninf = -1/0;

echo(nan);
echo(inf);
echo(ninf);

// validate step values
for(i=[0:nan:0]) { echo("NAN", i); }
for(i=[0:inf:0]) { echo("INF", i); }
for(i=[0:ninf:0]) { echo("-INF", i); }

for(i=[0:nan:1]) { echo("NAN", i); }
for(i=[0:inf:1]) { echo("INF", i); }
for(i=[0:ninf:1]) { echo("-INF", i); }

for(i=[1:nan:0]) { echo("NAN", i); }
for(i=[1:inf:0]) { echo("INF", i); }
for(i=[1:ninf:0]) { echo("-INF", i); }

// validate begin / end values
for(i = [0:inf]) {}
for(i = [0:ninf]) {}
for(i = [inf:0]) {}
for(i = [ninf:0]) {}

for(i = [0:2:inf]) {}
for(i = [0:2:ninf]) {}
for(i = [inf:2:0]) {}
for(i = [ninf:2:0]) {}
for(i = [inf:2:inf]) {}
for(i = [ninf:2:ninf]) {}
for(i = [inf:2:ninf]) {}
for(i = [ninf:2:inf]) {}
