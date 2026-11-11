// A closed polyhedron whose faces all point inwards: the result must not be
// inside out (#7201).
points = [[0,0,0],[10,0,0],[10,10,0],[0,10,0],[0,0,10],[10,0,10],[10,10,10],[0,10,10]];
faces = [[0,1,2,3],[4,5,1,0],[7,6,5,4],[5,6,2,1],[6,7,3,2],[7,4,0,3]];
intersection() {
  polyhedron(points, [for (f = faces) [for (i = [len(f) - 1 : -1 : 0]) f[i]]]);
  cube(5);
}
