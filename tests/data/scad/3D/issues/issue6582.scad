// Issue #6582: CGAL assertion "itl != it->second.end()" in applyUnion3D.
//
// The polygon below contains three consecutive vertices A, B, C closer to each other
// than GRID_FINE (2^-20). This is what projection() of examples/Basics/projection.stl
// produces. When converting the extruded PolySet to a Nef polyhedron, vertex
// quantization used to snap B onto A's grid cell while leaving C in place, which flips
// the long skinny cap triangle (B, C, [15,-15]) and yields a self-intersecting (corrupt)
// Nef polyhedron. Unioning that with any object located near B then failed.
translate([0, 0, -20])
  linear_extrude(1)
    difference() {
      square(30, center=true);
      polygon([
        [-1, -9.95],
        [5.9902668e-6, -9.99999989569187], // A
        [5.9977174e-6, -10],               // B
        [8.0019236e-6, -10],               // C
        [1, -9.95],
        [0, 10]
      ]);
    }

translate([8/1048576, -10, 0])
  cube(1);
