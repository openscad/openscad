difference() {
  linear_extrude(height = 2, twist = 0, convexity=2) {
    polygon(points = [[0, 0],
                      [1, -1],
                      [2, 0.2],
                      [2, -0.2],
                      [1, 1]]);
  }
  cube(center=true);
}
