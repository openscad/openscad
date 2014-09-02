// The inner cube won't render correctly in OpenCSG mode as long as this bug is present
// Note: This causes a different bug in unstable:
// If we render a preview first, the render() node will be cached as a PolySet. This will
// cause the same problems as in issue495.scad. If we clear cache and render using CGAL,
// it doesn't trigger the bug since we stay in CGAL all the time
difference() {
  render(convexity=2) difference() {
       cube(20, center = true);
       cube(10, center = true);
  }
  translate([0, 0, 15]) cube(30, center=true);
}
