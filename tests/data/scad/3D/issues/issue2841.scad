$fn = 7;
minkowski() {
    cube(0.1);
    union() {
        translate([5, 0, 0]) cylinder(h=10, r=5, center=true);
        cylinder(h=5, r=10, center=true);
    }
}
