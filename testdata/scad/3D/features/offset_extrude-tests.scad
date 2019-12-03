$fn=40;
size = 15;
off = size / 8;

Expand(0)
ExampleShape();

translate([0, size * 6])
offset_extrude(size, r = -off)
ExampleShape();

Expand(size * 2)
square([size,size], center = true);

translate([size * 2, size * 6])
offset_extrude(size, r = -off)
square([size,size], center = true);

module Expand(x) {
    translate([x, 0])
    offset_extrude(size, r = off)
    children();

    translate([x, size * 2])
    offset_extrude(size, delta = off, chamfer = true)
    children();

    translate([x, size * 4])
    offset_extrude(size, delta = off, chamfer = false)
    children();
}

module ExampleShape() {
    difference() {
        circle(size / 2);
        translate([0, -size / 2]) circle(size / 2);
    }
}