// 3D grid of cubes that overlap each neighbour
stride=0.8;

intersection() {
  union() {
    for (i=[0:3],j=[0:3])
      translate([i*stride, j*stride, 0])
        cube(1);
  }
  cube(10, center=1); // Just to ensure fast-union doesn't drop the union above we're trying to test
}
