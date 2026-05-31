// Regression test: osimport() dpi default must be SVG_DEFAULT_DPI (72.0),
// not 1.0. At dpi=1.0 the shape would be 72x smaller and near-invisible.
linear_extrude(height = 2, center = true)
    import("../../../svg/stroke-square.svg", center = true);
