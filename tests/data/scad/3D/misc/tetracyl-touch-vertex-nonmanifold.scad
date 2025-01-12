h = 1;

// A cylinder with a triangular base and a slim waist which is collapsed into a single vertex.
// This is a non-manifold object since the collapsed vertices are not distinct.
polyhedron(
  points=[
    [0,1,0],
    [cos(210), sin(210),0],
    [cos(330), sin(330),0],

    [0,0,h],

    [0,1,2*h],
    [cos(210), sin(210),2*h],
    [cos(330), sin(330),2*h],
  ],
  faces=[
    [0,1,2],
    [1,0,3],
    [0,2,3],
    [2,1,3],

    [3,4,5],
    [3,5,6],
    [3,6,4],
    [6,5,4],
  ]);
