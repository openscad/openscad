module roundedBox(size, radius)
{
  hull() {
    for (x = [-1, 1], y = [-1, 1], z = [-1, 1]) {
      translate([x * (size[0] / 2 - radius), y * (size[1] / 2 - radius), z * (size[2] / 2 - radius)])
        sphere(r=radius);
    }
  }
}


