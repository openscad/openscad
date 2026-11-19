// segments= takes precedence over $fn: 8 vertices per ring, not 40.
linear_extrude(height = 10, twist = 90, slices = 1, segments = 8, $fn = 40) square(10);
