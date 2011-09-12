module convex2dSimple() {
    hull() {
        translate([15,10]) circle(10);
        circle(10);
    }
}

module concave2dSimple() {
    hull() {
        translate([15,10]) square(2);
        translate([15,0]) square(2);
        square(2);
    }
}

// Works correctly
module convex2dHole() {
    hull() {
        translate([15,10,0]) circle(10);
        difference() {
            circle(10);
            circle(5);
        }
    }
}


convex2dHole();
translate([40,0,0]) convex2dSimple();
translate([0,-20,0]) concave2dSimple();
