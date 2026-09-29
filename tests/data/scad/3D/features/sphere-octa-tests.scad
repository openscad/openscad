// style="octa": subdivided octahedron spheres. Vertices on all six poles and
// the equators match circle()/cylinder() with the same number of fragments.
// Laid out on a grid, left to right and then front to back.

// Row 1: radius and diameter
sphere(style="octa");
translate([14,0,0]) sphere(5, style="octa");
translate([28,0,0]) sphere(r=5, style="octa");
translate([42,0,0]) sphere(d=10, style="octa");
translate([56,0,0]) sphere(r=1, d=10, style="octa");  // warning: d overrides r

// Row 2: $fn
translate([0,14,0]) sphere(5, $fn=3, style="octa");    // rounded up to 4 fragments, an octahedron
translate([14,14,0]) sphere(5, $fn=10, style="octa");  // rounded up to 12 fragments
translate([28,14,0]) sphere(5, $fn=16, style="octa");
translate([42,14,0]) sphere(5, $fn=24, style="octa");
translate([56,14,0]) sphere(5, $fn=0.1, style="octa"); // $fn below 3 acts as 3, an octahedron

// Row 3: $fa/$fs, and the other styles for comparison
translate([0,28,0]) sphere(5, $fa=20, $fs=0.3, style="octa");
translate([14,28,0]) sphere(5, $fa=30, $fs=0.3, style="octa");
translate([28,28,0]) sphere(5, $fa=40, $fs=0.3, style="octa");
translate([42,28,0]) sphere(5, style="orig");
translate([56,28,0]) sphere(5, style="bogus");         // warning, falls back to "orig"

// Row 4: degenerate input, and fitting cylinders
translate([0,42,0]) sphere(r=0, style="octa");         // r <= 0: no geometry and no warning
// Octa spheres fit cylinders on all three axes without slivers.
translate([14,42,0]) {
  sphere(4, $fn=16, style="octa");
  cylinder(r=4, h=5, $fn=16);
  rotate([90,0,0]) cylinder(r=4, h=5, $fn=16);
  rotate([0,90,0]) cylinder(r=4, h=5, $fn=16);
}
