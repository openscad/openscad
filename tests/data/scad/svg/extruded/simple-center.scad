linear_extrude(1, center = true)
import("../../../svg/simple.svg", center = true);

color("red") cylinder(r = 2, h = 0.4, center = true, $fn = 32);
color("green") difference() {
    cube([25, 25, 0.2], center = true);
    cube([23, 23, 1], center = true);
}

