// rotate([90,0,0]) results in a matrix with some close-to-zero values
// Transforming an empty boundingbox by this matrix has caused a problem resulting
// in the union being evaluated to nothing.
rotate([90,0,0]) 
difference() {
  cube(60, center=true);
  union() {
    translate([0,0,40]) cube(50, center=true);
    cube(0);
  }
}
