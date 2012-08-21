// Test translation by NaN and Infinity
// cube()s should not be rendered

// NaN
sphere();
rotate([0, 0, asin(1.1) ]) cube();

// Infinity (as of 2012-08 this is detected as NaN)
translate([4,0,0]) {
	sphere();
	rotate([0, 0, 1/0]) cube();
}
