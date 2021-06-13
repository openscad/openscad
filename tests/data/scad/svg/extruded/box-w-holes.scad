// Box should have 50 mm sides
// Holes should be 5 mm in diameter
//

linear_extrude(height = 2, center = true)
    import("../../../svg/box-w-holes.svg", center=true, dpi=72);

color("black") difference() {
    cube([56, 56, .1], center = true);
    cube([55, 55, 1], center = true);
}

color("red") {
    translate([15, 15, 0]) cylinder(r = 2, h = .1, $fn = 32);
    translate([15, -15, 0]) cylinder(r = 2, h = .1, $fn = 32);
    translate([-15, 15, 0]) cylinder(r = 2, h = .1, $fn = 32);
    translate([-15, -15, 0]) cylinder(r = 2, h = .1, $fn = 32);
}
