// No Children
extrude() 
{
}

// One child
extrude()
{
    square();
}

// One child from for loop
extrude() for (i=[0:0],union=false)
{
    translate([0,0,i])
    square();
}

// Overlapping
extrude()
{
    square(20);
    square(10);
}

// Intersecting
extrude()
{
    rotate([0,20,0])
    translate([0,0,1])
    square(10);
    square(10);
}

// Fewer points
extrude(interpolate=false)
{
    circle($fn=5);
    translate([0,0,10])
    circle($fn=3);
}

// More points
extrude(interpolate=false)
{
    circle($fn=3);
    translate([0,0,10])
    circle($fn=5);
}

// More points many slices
extrude(interpolate=false)
{
    circle($fn=5);
    translate([0,0,10])
    circle($fn=5);
    translate([0,0,20])
    circle($fn=5);
    translate([0,0,30])
    circle($fn=6);        
}

// Mirrored Z
extrude()
{
    square(10);
    translate([0,0,5])
    mirror([0,0,1])
    square(10);
}

// Mirrored X
/*
translate([0,0,0])
extrude()
{
    square(10);
    translate([0,0,5])
    mirror([1,0,0])
    square(10);
}

This fails but in the same way as the existing linear extrude:
linear_extrude(height=30,center=true, twist=180,slices=1,segments=2)
square(10,center=true);
*/

// Mirrored Y
translate([30,0,0])
extrude()
{
    square(10);
    translate([0,0,5])
    mirror([0,1,0])
    square(10);
}

// Mirrored XYY
extrude()
{
    square(10);
    translate([0,0,5])
    mirror([1,1,1])
    square(10);
}

// Spin it round
translate([60,0,0])
scale([8,8,1])
extrude()
{
    polygon([[0,0],[0,1],[1,1],[1,0]],[[0,1,2,3]]);    
    translate([0,0,5])
    rotate([0,0,90])
    polygon([[0,0],[0,1],[1,1],[1,0]],[[0,1,2,3]]);   
    translate([0,0,10])
    rotate([0,0,180])
    polygon([[0,0],[0,1],[1,1],[1,0]],[[0,1,2,3]]);     
    translate([0,0,20])
    rotate([0,0,270])
    polygon([[0,0],[0,1],[1,1],[1,0]],[[0,1,2,3]]);         
}

// Exercise it with a fine rotating circle, with a hole in the middle
translate([0,30,0])
extrude() for (i=[0:30:360],union=false)
{
    translate([i/80,i/60,i/10])
    rotate([i/8,i/3,0])
    rotate([0,0,i/2])
    difference()
    {
        circle(5,$fn=30);
        circle(4,$fn=3);
    }
}

// Difference between two extruded hoops, with holes in
module donut()
{
extrude(convexity=6) 
for (i=[0:20:360],union=false)
{
    rotate([0,0,i])
    translate([10,0,0])
    rotate([90,0,0])
    difference()
    {
        circle(5,$fn=30);
        circle(4,$fn=20);
    }
}
}

translate([30,30,0])
difference()
{
    donut();
    rotate([0,90,0])
    donut();
}

// Union of two donuts
translate([70,30,0])
rotate([0,0,-90])
difference()
{
union()
{
    donut();
    rotate([0,90,0])
    donut();
}
translate([0,-10,0])
cube([20,20,100]);
}




