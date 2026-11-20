// segments= subdivides the outline even for a straight extrusion,
// which is otherwise never subdivided: each side of the square is split
// in two, giving 8 vertices per ring.
linear_extrude(height = 10, segments = 8) square(10);
