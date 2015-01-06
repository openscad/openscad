module erode(r) {
    difference() {
        children();
        minkowski(convexity=2) {
            difference() {
                cube(14, center=true);
                children();
            }
            cube(2*r, center=true);
        }
    }
}

erode(2) cube(10, center=true);

