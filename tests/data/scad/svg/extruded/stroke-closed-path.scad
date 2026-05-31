// Regression test for closed SVG paths with stroke=true.
// A closed path (Z-terminated) must be imported as an outline so that
// linear_extrude and other 2D operations produce non-empty geometry.
linear_extrude(height = 2, center = true)
    import("../../../svg/stroke-square.svg", center = true, dpi = 96, stroke = true);
