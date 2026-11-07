// An outline that already has at least segments= vertices is left
// unchanged: the 12-sided circle keeps its 12 vertices.
linear_extrude(height = 10, segments = 4) circle(r = 5, $fn = 12);
