// The inner cube won't render correctly in OpenCSG mode as long as this bug is present
difference() {
  render(convexity=2) difference() {
       cube(20, center = true);
       cube(10, center = true);
  }
  translate([0, 0, 15]) cube(30, center=true);
}
