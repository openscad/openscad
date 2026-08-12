// Animation source for test_parallel_animation.py.
// Cheap geometry that still differs per frame, so frames are distinguishable
// but the test stays fast.
translate([$t * 10, 0, 0]) cube(5);
sphere(r = 3, $fn = 16);
