// The last four points aren't used by any face, so the result should be the same
// as without them: a pyramid with its corners rounded by the cube, not a box.
minkowski() {
  cube(1);
  polyhedron(
    points=[[1, 1, 0], [-1, 1, 0], [-1, -1, 0], [1, -1, 0], [0, 0, 2],
            [1, 1, 2], [-1, 1, 2], [-1, -1, 2], [1, -1, 2]],
    faces=[[0, 1, 2, 3], [0, 4, 1], [1, 4, 2], [2, 4, 3], [3, 4, 0]]
  );
}
