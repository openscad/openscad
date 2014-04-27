// In CGAL mode, there should be a cavity
// The problem appears to be that we don't create a correct Nef Polyhedron from
// the polyset, probably already being wrong when converting to a Polyhedron
difference() {
  polyhedron(convexity=2, faces=[[2,1,0],[3,1,2],[4,0,1],[4,1,5],[4,2,0],[6,2,4],[6,3,2],[7,3,6],[5,1,3],[5,3,7],[6,4,5],[6,5,7],[9,10,8],[9,11,10],[8,12,9],[9,12,13],[10,12,8],[10,14,12],[11,14,10],[11,15,14],[9,13,11],[11,13,15],[12,14,13],[13,14,15]],
points=[[-5,-5,-5],[-5,-5,5],[-5,5,-5],[-5,5,5],[5,-5,-5],[5,-5,5],[5,5,-5],[5,5,5],[-2.5,-2.5,-2.5],[-2.5,-2.5,2.5],[-2.5,2.5,-2.5],[-2.5,2.5,2.5],[2.5,-2.5,-2.5],[2.5,-2.5,2.5],[2.5,2.5,-2.5],[2.5,2.5,2.5]]);
  translate([0,0,10]) cube(20, center=true);
}