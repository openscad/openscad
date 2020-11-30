// This file is a part of openscad. Everything implied is implied.
// Author: Alexey Korepanov <kaikaikai@yandex.ru>

echo(version=version());

// These are examples for the `roof` function, which builds
// "roofs" on top of 2d sketches. A roof can be constructed using
// either straight skeleton or Voronoi diagram (see below).
//
// Under the hood, to construct straight skeletons we use cgal,
// while for Voronoi diagrams we use boost::polygon.
//
// It may be worth noting that with our current implementation
// Voronoi diagrams are much faster (10x - 100x) to compute
// than straight skeletons.

// some 2d sketch
module sketch() {
    polygon(points=[[-5,-1],[-0.15,-1],[0,0],[0.15,-1],[5,-1],
    [5,-0.1],[4,0],[5,0.1],[5,1],[-5,1]]);
}

// straight skeleton roof
roof(method = "straight skeleton") sketch();

// Voronoi diagram roof (the default)
translate([0,-5,0])
roof(method = "voronoi diagram") sketch();

// Voronoi diagram respects discretization parameters
// $fa, $fs and $fn:
translate([0,-8,0])
roof(method = "voronoi diagram", $fn=4) sketch();

// A nice application is beveling of fonts:
translate([6,0,0])
roof(method = "straight skeleton")
text("straight skeleton", size = 2, halign = "left", valign = "center");

translate([6,-7,0])
roof(method = "voronoi diagram")
text("Voronoi diagram", size = 2, halign = "left", valign = "center");