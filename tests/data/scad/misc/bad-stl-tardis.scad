/*
sorry, this triangulation does not deal with
 intersecting constraints
CGAL error: assertion violation!
Expression : false
File       : ../libraries/install/include/CGAL/Constrained_triangulation_2.h
Line       : 636
Explanation: 
Refer to the bug-reporting instructions at http://www.cgal.org/bug_report.html
2013-12-29 21:15:18.937 OpenSCAD[35590:507] ERROR: CGAL NefPolyhedron Triangulation failed
2013-12-29 21:15:19.104 OpenSCAD[35590:507] ERROR: CGAL NefPolyhedron->Polyhedron conversion failed.

This has been fixed, but keep this test for future reference
*/
render() import("bad-stl-tardis.stl");
