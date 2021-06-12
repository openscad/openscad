translate([0,-20]) intersection() {
  circle(r=5);
  square(8,center=true);
}

// Intersecting something with nothing
translate([-10,0]) intersection() {
  circle(r=5);
  square(0,center=true);
}

// Non-geometry (echo) statement as first child should be ignored
translate([0,20]) intersection() {
  echo("difference-tests");
  circle(r=5);
  square(8,center=true);
}

// intersection with 1 operand
translate([20,-20]) intersection() {
	translate([10,0]) circle(r=15);
}

// intersection with 2 operands
translate([20,0]) intersection() {
	translate([10,0]) circle(r=15);
	rotate(120) translate([10,0]) circle(r=15);
}

// intersection with 3 operands
translate([20,20]) intersection() {
	translate([10,0]) circle(r=15);
	rotate(120) translate([10,0]) circle(r=15);
	rotate(240) translate([10,0]) circle(r=15);
}

// intersection_for
translate([0,0]) intersection_for (a = [0:60:359.99]) {
	translate([cos(a),sin(a)]*10) circle(r=15);
}
