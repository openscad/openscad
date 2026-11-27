// Group at the top level.
group(name="octo_dome") color("blue") {
    difference() {
      sphere(r=12, $fn=8);
      translate([-500, -500, -1000]) cube(1000);
    }
  }

// Group with one module applied to it.
color("red") group(name="flag")
    translate([0, 0, 12 * cos(360 / 16)]) {
      translate([0, -0.25, 8.5]) cube([5, 0.5, 3]);
      translate([-0.5, -0.50, 0]) cube([1, 1, 12]);
    }

// Empty
group(){}
