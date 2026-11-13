// A closed polyhedron whose faces all point inwards (#7201). Subtracting it
// must cut away the corner of the cube; the result is not a whole cube.
points = [[0,0,0],[10,0,0],[10,10,0],[0,10,0],[0,0,10],[10,0,10],[10,10,10],[0,10,10]];
faces = [[0,1,2,3],[4,5,1,0],[7,6,5,4],[5,6,2,1],[6,7,3,2],[7,4,0,3]];
difference() {
  translate([5, 5, 5]) cube(10);
  polyhedron(points, [for (f = faces) [for (i = [len(f) - 1 : -1 : 0]) f[i]]]);
}
