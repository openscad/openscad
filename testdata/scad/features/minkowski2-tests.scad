module roundedBox2dSimple() {
    minkowski() {
        square([10,10]);
        circle(r=5);
    }
}

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
            square([10,10], center=true);
            square([8,8], center=true);
        }
        circle(r=2);
    }
}

translate([-20,5,0]) roundedBox2dHole();
translate([0,0,0]) roundedBox2dCut();
translate([25,0,0]) roundedBox2dSimple();
