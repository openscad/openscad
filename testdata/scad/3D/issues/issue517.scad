//
// Test that rotating a cube 90 degrees is accurate.
// Due to symmetry the result should be empty.
//
render() difference() {
    cube(10, true);

    rotate([90, 0, 0])
        cube(10, true);
}
//
// Check that rotating a cube 120 degrees around its diagonal is accurate.
//
render() translate([20, 0]) difference() {
    cube(10, true);

    rotate(120, [1, 1, 1])
        cube(10, true);
}
