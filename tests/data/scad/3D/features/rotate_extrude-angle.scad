$fa=15;
$fs=4;

module face(x) {
  translate([x,0]) difference() {
    square(10,center=true);
    square(5,center=true);
  }
}

module face2() {
  translate([5,0]) square(5); 
}

// test negative partial angles and geometries on -X side
rotate_extrude(angle=45) face(10);
rotate_extrude(angle=45) face(-10);
rotate_extrude(angle=-45) face(21);
rotate_extrude(angle=-45) face(-21);

// test small angles, angle < $fa, render a single segment
rotate([0,0,90]) {
  rotate_extrude(angle=5) face(10); 
  rotate_extrude(angle=5) face(-10); 
  rotate_extrude(angle=-5) face(21); 
  rotate_extrude(angle=-5) face(-21);
}

// show nothing
rotate_extrude(angle=0) face(5); // 0 angle

// various angles treated as full circle
translate([-40,40]) rotate_extrude() face2(); // unspecified
translate([0,40]) rotate_extrude(angle=0/0) face2(); // NaN
translate([40,40]) rotate_extrude(angle=1/0) face2(); // Infinity
translate([-40,0]) rotate_extrude(angle=-1/0) face2(); // -Infinity
translate([40,0]) rotate_extrude(angle=360) face2(); // 360
translate([-40,-40]) rotate_extrude(angle=-360) face2(); // -360
translate([0,-40]) rotate_extrude(angle=1000) face2(); // > 360
translate([40,-40]) rotate_extrude(angle=-1000) face2(); // < -360
