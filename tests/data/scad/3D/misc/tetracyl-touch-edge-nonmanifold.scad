h = 1;

// A cylinder with a triangular base and a slim waist which is collapsed into a single edge.
// This is a non-manifold object since the collapsed vertices are not distinct.
polyhedron(
  points=[
    [0,1,0],
    [cos(210), sin(210),0],
    [cos(330), sin(330),0],

    [0,0.1,h],
    [0.1*cos(270), 0.1*sin(270),h],

    [0,1,2*h],
    [cos(210), sin(210),2*h],
    [cos(330), sin(330),2*h],
  ],
  faces=[
    [0,1,2],
    [1,0,3],[1,3,4],
    [0,2,4],[0,4,3],
    [2,1,4],

    [4,3,5],[4,5,6],
    [4,6,7],
    [3,4,7],[3,7,5],
    [7,6,5],
  ]);
