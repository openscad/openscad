// segments=0 disables subdivision, even with twist: the square keeps its
// 4 vertices per ring.
linear_extrude(height = 10, twist = 90, slices = 1, segments = 0) square(10);
