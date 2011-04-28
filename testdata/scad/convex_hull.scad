// Works correctly
module convex2dSimple() {
    hull() {
        translate([15,10]) circle(10);
        circle(10);
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

// 3d not currently implemented
module convex3dSimple() {
    hull() {
        translate([15,10]) cylinder(r=10);
        cylinder(r=10);
    }
}

// 3d not currently implemented
module convex3dHole() {
    hull() {
        translate([15,10,0]) cylinder(10);
        difference() {
            cylinder(10);
            cylinder(5);
        }
    }
}


convex2dHole();
translate([40,0,0]) convex2dSimple();
translate([0,40,0]) convex3dHole();
translate([40,40,0]) convex3dSimple();
