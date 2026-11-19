// With twist, segments= replaces the default $fs/$fa based subdivision
// (which would split each side into 5): 8 vertices per ring.
linear_extrude(height = 10, twist = 90, slices = 1, segments = 8) square(10);
