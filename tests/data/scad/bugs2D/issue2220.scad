hull() {
    translate([0, 10])
        draw_circle();
    translate([10,0])
        draw_circle();
}

module draw_circle() {
    rotate([0, 0, 358.987]) {
        rotate([0, 0, -358.987]) {
            circle(d=5, $fn=4);
        }
    }
}
