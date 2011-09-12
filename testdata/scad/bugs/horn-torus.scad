// This model causes a CGAL assertion in CGAL_Nef_polyhedron3(CGAL_Polyhedron) constructor.
// One cause of this error could be that the grid handling in PolySet degenerated the original
// mesh into a non-manifold one.

rotate_extrude($fn = 24) translate ([ 1, 0, 0 ]) circle (r = 1);
