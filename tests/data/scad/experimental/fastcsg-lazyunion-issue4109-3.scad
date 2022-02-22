cube(); 
translate([10,0])
  cube(2);

translate([10,10])
  union(){ 
    sphere();
    translate([10,0])
      cylinder();
  }
