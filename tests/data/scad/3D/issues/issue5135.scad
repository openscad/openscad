v = [[0, 0, 0], [10, 0, 0], [10, 10, 0], [0, 10, 0], [0, 0, 10], [10, 0, 10], [10, 10, 10], [0, 10, 10], [0,0,1e-14]];
f = [[0, 1, 2, 3], [8, 4, 5, 1], [1, 5, 6, 2], [2, 6, 7, 3], [3, 7, 4, 0], [6, 5, 4, 7]];
polyhedron(v, faces=f);
