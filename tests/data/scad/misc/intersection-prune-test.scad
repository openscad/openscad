// This tree cannot be pruned inline, but will still result in an empty CSG tree
// Crashes OpenSCAD-2011.12. Bug fixed in 14e4f3bb
intersection() {
 union() {
  cube();
  translate([4,0,0]) cube();
 }
 translate([2,0,0]) cube();
}
