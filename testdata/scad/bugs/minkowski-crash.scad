/*
  Originally reported by nop head 20120107:
  This causes a CGAL assertion in minkowski on some platforms and CGAL versions:
  o CGAL-3.6, 3.8 Linux
  o Windows (OpenSCAD-2011.12 binaries)

  The problem is that minkowski leaves the target polyhedron in a corrupt state
  causing a crash. This is worked around in CGALEvaluator::process().
  
  CGAL-3.9 appears to just process forever.
*/
$fn = 30;
minkowski() {
	union() {
		cube([10, 10, 10], center=true);

		cylinder(r=2, h=15, center=true);

		rotate([90, 0, 0])
			cylinder(r=2, h=15, center=true);

		rotate([0, 90, 0])
			cylinder(r=2, h=15, center=true);
	}
	sphere(3);
}
