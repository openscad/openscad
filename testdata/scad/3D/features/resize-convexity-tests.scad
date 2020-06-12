$fn = 20;

difference() {
    resize([50, 50, 15], convexity = 2) {
        difference() {
            cube([10, 10, 5], center = true);
            cylinder(8, center = true);
        }
    }
    translate([15, 15, 0]) cube([10, 10, 20], center = true);
}
