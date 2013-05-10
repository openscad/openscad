difference() {
    linear_extrude(height=40, scale=[0, 0]) {
        square(10, center=true);
        translate([10,0]) circle(10);
    }
 translate([0,0,35])   sphere(10);
}

/*
Test case ideas:
o off-center starting point
o Concave polygon
o Disjoint polygons
o multi-rotation twist
o zero scales, zero scales in only one axis (for the above cases)
o boolean operations on scaled extrusion (including zero scale)
*/
