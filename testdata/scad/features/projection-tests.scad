// Empty
projection();
// No children
projection() { }
// 2D child
projection(cut=true) { square(); }

projection(cut=false) cube(10);
projection(cut=true) translate([20,0,0]) cube(10, center=true);
// Boundary case: clipping the top of a cube
translate([0,20,0]) projection(cut=true) translate([0,0,-4.999999]) cube(10, center=true);

// holes
translate([0,-10,0]) projection(cut=true) {
  union() {
      difference() { cube(5,center=true); cube(4,center=true); }
      translate([2.1,2.1]) difference() { cube(5,center=true); cube(4,center=true); }
  }
}
