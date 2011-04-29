
// Rounded box using 3d minkowski
module roundedBox3dSimple() {
    minkowski() {
        cube([10,10,5]);
        cylinder(r=5, h=5);
    }
}

// Currently segfaults
module roundedBox3dCut() {
    minkowski() {
        difference() {
            cube([10,10,5]);
            cube([5,5,5]);
        }
        cylinder(r=5, h=5);
    }
}

// Currently segfaults
module roundedBox3dHole() {
    minkowski() {
        difference() {
            cube([10,10,5]);
            translate([2,2,-2]) cube([6,6,10]);
        }
        cylinder(r=2);
    }
}

// Works correctly
module roundedBox2dSimple() {
    minkowski() {
        square([10,10]);
        circle(r=5);
    }
}

// Works correctly
module roundedBox2dCut() {
    minkowski() {
        difference() {
            square([10,10]);
            square([5,5]);
        }
        circle(r=5);
    }
}

// Not quite correct, result does not contain a hole, since the impl currently returns the outer boundary of the polygon_with_holes.
module roundedBox2dHole() {
    minkowski() {
        difference() {
            square([10,10]);
            translate([2,2]) square([6,6]);
        }
        circle(r=2);
    }
}

translate([-25,0,0]) roundedBox2dHole();
translate([0,0,0]) roundedBox2dCut();
translate([25,0,0]) roundedBox2dSimple();
translate([-25,25,0]) roundedBox3dHole();
translate([0,25,0]) roundedBox3dCut();
translate([25,25,0]) roundedBox3dSimple();
