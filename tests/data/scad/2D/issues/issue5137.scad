projection() {
  polyhedron(
    points=[
  [-2, -0.7, -0.1],
  [-1.9, -0.7, -0.1],
  [-1.9, -0.6901, -0.1],
  [-2, -0.7, 0],
  [-1.9, -0.7, 0],
  [-1.9, -0.6901, 0],
  [-2, 0, -0.1],
  [-2, 0, 0],
  [-1, -0.6901, -0.1],
  [-1, -0.6901, 0],
  [-1, 0, -0.1],
  [-1, 0, 0],
    ],
    faces=[
  [1,2,0,],
  [3,1,0,],
  [5,2,1,],
  [6,0,2,],
  [8,2,5,],
  [4,1,3,],
  [5,1,4,],
  [3,5,4,],
  [7,3,0,],
  [5,3,7,],
  [7,0,6,],
  [10,6,2,],
  [11,5,7,],
  [6,11,7,],
  [10,2,8,],
  [9,10,8,],
  [5,9,8,],
  [9,5,11,],
  [11,6,10,],
  [10,9,11,],
    ],
  );
  polyhedron(
    points=[
  [-2, -1, 3],
  [-1.9, -1, 3],
  [-1.9, -0.6901, 3],
  [-1.7, -0.6901, 3],
  [-1.7, -0.6901, 3.2],
  [-2, -1, 4],
  [-1.9, -1, 4],
  [-1.9, -0.6901, 4],
  [-2, 0, 3],
  [-1.7, 0, 3],
  [-1.7, 0, 3.2],
  [-2, 0, 4],
  [-1, -0.6901, 3.2],
  [-1, -0.6901, 4],
  [-1, 0, 3.2],
  [-1, 0, 4],
    ],
    faces=[
  [2,0,1,],
  [0,5,1,],
  [6,2,1,],
  [8,0,2,],
  [3,2,7,],
  [4,3,7,],
  [6,1,5,],
  [2,6,7,],
  [7,6,5,],
  [11,5,0,],
  [7,5,11,],
  [12,4,7,],
  [9,2,3,],
  [11,0,8,],
  [8,2,9,],
  [10,8,9,],
  [10,11,8,],
  [10,3,4,],
  [9,3,10,],
  [13,7,11,],
  [14,11,10,],
  [12,7,13,],
  [14,10,4,],
  [14,4,12,],
  [11,15,13,],
  [15,11,14,],
  [15,12,13,],
  [14,12,15,],
    ],
  );
}