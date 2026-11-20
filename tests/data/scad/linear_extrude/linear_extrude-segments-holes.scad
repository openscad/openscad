// segments= applies to each outline separately: both the outer square and
// the hole get 8 vertices per ring.
linear_extrude(height = 10, segments = 8)
  difference() {
    square(10, center = true);
    square(4, center = true);
  }
