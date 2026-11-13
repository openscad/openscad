// The viewBox of this SVG starts at 100,50 instead of 0,0.
// With center = true the L shape (20mm x 10mm, bar on top, stem on the
// left) must still be centered on the origin (red) and fit inside the
// green frame.

linear_extrude(1, center = true)
import("../../../svg/viewbox-offset.svg", center = true);

color("red") cylinder(r = 1, h = 0.4, center = true, $fn = 32);
color("green") difference() {
    cube([23, 13, 0.2], center = true);
    cube([21, 11, 1], center = true);
}
