// The viewBox of this SVG starts at 100,50 instead of 0,0.
// With center = true the L shape (20mm x 10mm, bar on top, stem on the
// left) must still be centered on the origin (dot) and fit inside the
// frame.

import("../../svg/viewbox-offset.svg", center = true);

circle(r = 1, $fn = 32);
difference() {
    square([23, 13], center = true);
    square([21, 11], center = true);
}
