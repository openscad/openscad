//
// Reported by Triffid Hunter.
// Causes a crash in CSGTermNormalizer::normalizePass()
//

difference() {
  union() {
    translate([0, -20, 0]) cube([30, 2, 40]);
    cube();
  }

  translate([15.5, -19, 14]) {
    cylinder(r=5, h=2);
    rotate([-90, 0, 0]) difference() {
      translate([0, 0, 2]) cylinder(r=2, h=3);
      translate([0, 0, 4]) cylinder(h=2);
    }
  }
}
