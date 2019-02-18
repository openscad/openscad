// Hollow cube.
// STL export is wrong (inner cube is positive instead of negative)
// This is fixed by applying and using the patch to CGAL containing
// convert_all_inner_shells_to_polyhedron()
difference() {
    cube(20, center = true);
    cube(10, center = true);
}
