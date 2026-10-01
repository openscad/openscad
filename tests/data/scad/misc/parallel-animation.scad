// Animation source for test_parallel_animation.py.
// Cheap to render but visibly different in every frame, so a dropped, duplicated
// or reordered frame changes the output rather than hiding in identical pixels.
rotate([0, 0, $t * 360]) translate([8, 0, 0]) cube(6, center = true);
sphere(r = 5, $fn = 24);
