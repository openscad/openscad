difference() {
    rotate_extrude(convexity=4, $fn=8)
    translate([5,0,0]) difference() {
        circle(r=2);
        circle(r=1);
    }
    translate([-5,-5,5]) cube(10, center=true);
}
