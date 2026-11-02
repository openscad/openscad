// Uncolored geometry keeps the export stroke and fill.
square(10);

// Overlapping shapes of one color union into a single contour.
color("red") translate([20, 0]) square(10);
color("red") translate([25, 0]) square(10);

// A different color stays its own path and is painted after earlier colors.
color("blue") translate([0, 20]) square(10);

// Alpha is written on both the fill and the stroke.
color([0, 1, 0, 0.5]) translate([20, 20]) square(10);

// difference() keeps the first child's color.
difference() {
  color("magenta") translate([0, 40]) square(10);
  translate([3, 43]) square(4);
}

// An outer color() replaces colors nested inside it.
color("cyan") color("yellow") translate([20, 40]) square(10);
