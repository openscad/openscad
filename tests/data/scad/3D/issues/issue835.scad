difference() {
    cube([20,20,3], center=true);

    linear_extrude(height=10, center=true, convexity=2) {
        translate([5,0]) circle(r=3);
        translate([-5,5]) circle(r=3);
        translate([-5,-5]) circle(r=3);
    }
}
