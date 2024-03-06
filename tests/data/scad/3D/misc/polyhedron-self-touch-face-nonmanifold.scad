// A single volume touching itself (three pairs of coincident vertices forming a triangle).
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

    [0.4, 0.4, 0.5],
    [0.6, 0.4, 0.5],
    [0.5, 0.6, 0.5],
  ],
  faces=[
    [5,4,8],[5,8,9],
    [4,6,8],
    [6,10,8],
    [6,7,10],
    [7,9,10],
    [7,5,9],
    [10,9,8],
    
    [0,1,9],[0,9,8],
    [1,3,9],
    [3,10,9],
    [3,2,10],
    [2,8,10],
    [2,0,8],
    [8,9,10],
    
    [4,5,1],[4,1,0],
    [5,7,3],[5,3,1],
    [7,6,2],[7,2,3],
    [6,4,0],[6,0,2],
  ]);
