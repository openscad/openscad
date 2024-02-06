points=[
    [0,0], [1,0], [1,1], [0,1],
    [0.2,0.2], [0.8,0.2], [0.8,0.8], [0.2,0.8],
    [0.3,0.3], [0.7,0.3], [0.7,0.7], [0.3,0.7],
    [0.4,0.4], [0.6,0.4], [0.6,0.6], [0.4,0.6],
    [-0.5,-0.5], [0.5,-0.5], [0.5,0.5], [-0.5,0.5],
  ];

// Intersecting polygons
// According to the docs, the second contour should count as a hole, leaving us with a polygon with a chunk subtraced from a corner.
polygon(
  points=points,
  paths=[
    [0,1,2,3],
    [16,17,18,19],
  ]
);

// Polygon with one proper hole, and one intersecting hole
// The intersected hole ends up producing another polygon using and even/odd rule instead of acting as a hole
translate([2,0,0]) polygon(
  points=points,
  paths=[
    [0,1,2,3],
    [4,5,6,7],
    [16,17,18,19],
  ]
);

// More complex example with multiple holes, islands and intersections
translate([0,2,0]) polygon(
  points=points,
  paths=[
    [0,1,2,3],
    [4,5,6,7],
    [8,9,10,11],
    [12,13,14,15],
    [16,17,18,19],
  ]
);

