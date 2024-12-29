difference(){
  minkowski(convexity=2){
    difference(){
      cylinder(h=10, d=15);
      translate([0,0,-0.1]) cylinder(h=11, d=14);
    }
    sphere(1, $fn=8);
  }
  translate([0,0,15]) cube(20, center=true);
}
