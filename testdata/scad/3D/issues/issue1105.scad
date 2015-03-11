translate([-31.5, -30, 1]) {
    rotate(90) {
        translate([2, -51.5, 26]) {
        difference() {
        cube([4, 14, 4]);
                translate([-0.1,-1,2.1]) cube([2.1, 16, 2.1]);
        }
    }
    }
}
translate([3, -25, 1]) rotate_extrude($fn=12) polygon(points = [[0, 20], [0, 30], [7, 26]]);
translate([0, -28, 20]) cube([6, 3, 10]);
