module erode(r) {
    difference() {
        children();
        minkowski(convexity=3) {
            difference() {
                cube(25, center=true);
                children();
            }
            sphere(r);
        }
    }
}

module object() {
    translate([-5,0,0]) cube([10,5,5]);
    translate([0,-5,0]) cube([5,10,5]);
    cube([5,5,10]);
}

erode(1,$fn=12) object();
