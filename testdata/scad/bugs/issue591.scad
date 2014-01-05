// Creates the same "non-manifold" object (sharing one edge) using two techniques:
// o linear_extrude of two 2D objects
// o union of two linear_extrudes
//
// Subsequently cuts away the non-manifold part:
// -> the first technique fails, the second one succeeds
//
module cutoff() {
    difference() {
        children();
        translate([0,0,2.5]) cube(2, center=true);
    }
}

cutoff() {
    linear_extrude(height=3, scale=[0,1], convexity=2) {
        translate([1,0,0]) square(1,true);
        translate([-1,0,0]) square(1,true);
    }
}

translate([0,2,0]) cutoff() {
    linear_extrude(height=3, scale=[0,1]) translate([1,0,0]) square(1,true);
    linear_extrude(height=3, scale=[0,1]) translate([-1,0,0]) square(1,true);
}
