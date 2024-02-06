// Triangle
translate([2,0,0]) polygon([[0,0], [1,0], [1,1]]);

// Quad
translate([0,-2,0]) polygon(points=[[0,0], [1,0], [1,1], [0,1]]);

// Empty paths: Using points in order
translate([0,-4,0]) polygon(points=[[0,0], [1,0], [1,1], [0,1]], paths=[]);

// Concave polygon
translate([2,-2,0]) polygon([[0,0], [1,0], [0.8,0.5], [1,1], [0,1]]);

points = [[0,0], [0.5,-0.2], [1,0], [1.2,0.5], [1,1], [0.5,1.2], [0,1], [-0.2,0.5]];

// No paths: Using points in order
translate([-2,0,0]) polygon(points);

// Create two polygons using a single polygon() instantiation
// TODO: This doesn't match with the docs
translate([-2,-2,0]) polygon(points=points, paths=[[0,1,2,3], [4,5,6,7]]);


// Triangle with the same first and last vertex position
translate([2,-4,0]) polygon([[0,0], [1,0], [1,1], [0,0]]);

// Quad with hole
translate([-2,-4,0])
  polygon(points=[[0,0], [1,0], [1,1], [0,1], [0.2,0.2], [0.8,0.2], [0.8,0.8], [0.2,0.8]],
          paths=[[0,1,2,3],[4,5,6,7]]
);

// Constructing a polygon with a hole from two touching polygons
// TODO: This doesn't match with the docs
translate([0,0,0])
polygon(points = [[0,1], [0,0], [1,0], [1,1], [0.8,0.8], [0.8,0.2], [0.2,0.2], [0.2,0.8]],
        paths = [[7,6,5,4,3,2,1,0],[7,0,3,4] ]);


// Empty polygons
polygon();
polygon([]);
polygon([[],[]]);
polygon([[[]]]);

// Malformed polygons:

// Only a single vertex
translate([0,2,0]) polygon([[0,0]]);

// Only two vertices
translate([2,2,0]) polygon([[0,0],[1,1]]);

// Collinear vertices
translate([2,2,0]) polygon([[0,0],[1,1],[2,2]]);

polyhedron(points = undef, paths = [[1, 2, 3]]);
polyhedron(points = [[0,0,0],[1,1,1]], paths = undef);
polyhedron(points=[0], paths = [[0]]);
    
// FIXME: convexity
