// Triggers a CGAL assertion_exception after calling
// convert_to_Polyhedron() which, for some reason, we don't manage to
// crash, causing std::terminate() to be called.

render() { 
  import("stl-cgal-convert_to_Polyhedron-crash.stl");
} 
