// A single volume touching itself (two coincident vertices).
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

    [0.5, 0.5, 0.5],
  ],
  faces=[
    [5,4,8],
    [4,6,8],
    [6,7,8],
    [7,5,8],
    
    [0,1,8],
    [1,3,8],
    [3,2,8],
    [2,0,8],
    
    [4,5,1],[4,1,0],
    [5,7,3],[5,3,1],
    [7,6,2],[7,2,3],
    [6,4,0],[6,0,2],
  ]);
