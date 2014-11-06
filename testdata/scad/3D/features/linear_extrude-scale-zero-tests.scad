// test cases for linear extrude with scale
// by TakeItAndRun 2013

// syntax: linear_extrude(height=a, slices=b, twist=c, scale=[x,y])

a=3;
b=20;
c=0;
x=1;
y=1;

module linear_extrudes_of_different_shapes(a=a,b=b,c=c,x=x,y=y) {
  translate(00*[4,0,0])
  // linear_extrude of shape with hole
  linear_extrude(height=a, slices=b, twist=c, scale=[x,y])
    difference() {
      square(2,true); square(1,true);
    }
  
  translate(01*[4,0,0])
  // linear_extrude of disjoint polygons shapes
  linear_extrude(height=a, slices=b, twist=c, scale=[x,y]) {
    translate([1,0,0]) square(1,true);
    translate([-1,0,0]) square(1,true);
  }
  
  translate(02*[4,0,0])
  // linear_extrude with a coplanar face
  linear_extrude(height=a, slices=b, twist=c, scale=[x,y]) {
    translate([.5,0,0])square();
    translate([-.5,0,0])square();
  }
  
  translate(03*[4,0,0])
  // linear_extrude with internal hole and one coplanar edge
  linear_extrude(height=a, slices=b, twist=c, scale=[x,y])
  difference() {
    square(2,true);
    translate([-0.5,0,0]) square(1,true);
  }
}


// Test varying parameters
translate(00*[0,3,0])
linear_extrudes_of_different_shapes(c=0,x=0,y=y);
translate(01*[0,3,0])
linear_extrudes_of_different_shapes(c=0,x=x,y=0);
translate(02*[0,3,0])
linear_extrudes_of_different_shapes(c=0,x=0,y=0);
translate(03*[0,3,0])
linear_extrudes_of_different_shapes(c=180,x=0,y=y);
translate(04*[0,3,0])
linear_extrudes_of_different_shapes(c=180,x=x,y=0);
translate(05*[0,3,0])
linear_extrudes_of_different_shapes(c=180,x=0,y=0);
