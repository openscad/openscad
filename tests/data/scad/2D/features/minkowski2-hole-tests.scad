// HolePoly & Poly
minkowski() {
    difference() {
        square([20,20], center=true);
        square([10,10], center=true);
    }
    circle(r=1, $fn=16);
}

// Poly & HolePoly
translate([25,0]) minkowski() {
    circle(r=1, $fn=16);
    difference() {
        square([20,20], center=true);
        square([10,10], center=true);
    }
}

// IslandHolePoly
translate([0,25]) minkowski() {
    union() {
        difference() {
            square([20,20], center=true);
            square([10,10], center=true);
        }
        square([2,2], center=true);
    }
    circle(r=1, $fn=16);
}

// HolePoly & HolePoly
translate([25,25]) minkowski() {
    difference() {
        square([18,18], center=true);
        square([12,12], center=true);
    }
    difference() {
        circle(2, $fn=16);
        circle(1, $fn=16);
    }
}
