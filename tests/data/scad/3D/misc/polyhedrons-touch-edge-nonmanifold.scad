// Two cubes sharing an edge.
// This is a non-manifold object since the collapsed vertices are not distinct.
polyhedron(
  points=[
    [0, 0, 0],
    [1, 0, 0],
    [0, 1, 0],
    [1, 1, 0],
    [0, 0, 1],
    [1, 0, 1],
    [0, 1, 1],
    [1, 1, 1],

    [2, 1, 0],
    [1, 2, 0],
    [2, 2, 0],
    [2, 1, 1],
    [1, 2, 1],
    [2, 2, 1],
  ],
  faces=[
    [6,7,5,4],
    [0,1,3,2],
    [4,5,1,0],
    [5,7,3,1],
    [7,6,2,3],
    [6,4,0,2],
    [12,13,11,7],
    [3,8,10,9],
    [7,11,8,3],
    [11,13,10,8],
    [13,12,9,10],
    [12,7,3,9],
  ]);
