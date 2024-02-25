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

    [0.4, 0.5, 0.5],
    [0.6, 0.5, 0.5],

    [0.4, 0.5, 0.5],
    [0.6, 0.5, 0.5],
  ],
  faces=[
    [5,4,10,11],
    [4,6,10],
    [6,7,11,10],
    [7,5,11],
    
    [0,1,9,8],
    [1,3,9],
    [3,2,8,9],
    [2,0,8],
    
    [4,5,1,0],
    [5,7,3,1],
    [7,6,2,3],
    [6,4,0,2],
  ]);
