// Spheres joined to cylinders with the same $fn on all three axes, for $fn
// from 4 to 32. style="octa" fits without gaps or ledges, style="orig" does
// not. Octa (front) and orig (back).
for (style_offset = [["octa", -50], ["orig", 50]]) {
  style = style_offset[0];
  translate([0, style_offset[1], 0]) {
    for (i = [1:8]) {
      $fn = 4 * i;
      translate([30 * (5 - i), 0, 0]) {
        sphere(10, style=style);
        rotate([0, 180, 0]) cylinder(r=10, h=15);

        translate([0, 20, 0]) {
          sphere(10, style=style);
          rotate([0, -90, 0]) cylinder(r=10, h=15);
        }

        translate([0, -30, -10]) {
          sphere(10, style=style);
          rotate([-90, 0, 0]) cylinder(r=10, h=15);
        }
      }
    }
  }
}
