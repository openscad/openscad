// These are cases where solids share edges or vertices, which can be challenging
// for some libraries used for rendering (e.g. corefinement functions in CGAL's
// polygon mesh processing package) and must be worked around by OpenSCAD.
N = 5;

cube(1);
rotate([0, 0, 45]) cube(1);
rotate([0, 0, 90]) cube(1);

translate([2, 0, 0]) {
  union() {
    cube(1);
    translate([1, 1, 0]) cube(0.5);
  }
}

translate([4, 0, 0]) {
  difference() {
    cube(1);
    translate([1, 1, 0]) cube(1);
  }
}

translate([6, 0, 0]) {
  cube(1);
  translate([0.25, 0.25, 1]) cube(0.5);
}

translate([8, 0, 0]) {
  cube(1);
  translate([1, 1, 0]) cube(0.5);
}

translate([10, 0, 0]) {
  cube(1);
  translate([1, 1, 0.5]) cube(1);
}

translate([13, 0, 0]) {
  cube(1);
  translate([1, 0, 0]) cube(1);
}

translate([0, 4, 0]) {
  union()
    for(i=[1:N], j=[1:N], k=[1:N])
      translate([i,j,k]) cube();
}
