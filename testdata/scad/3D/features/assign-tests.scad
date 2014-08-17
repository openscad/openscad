for(i=[0:5]) {
  translate([i*i/2,0,0]) {
    cube(i);
    translate([0,-5,0]) assign(f=1.0*i/2) cube(f);
  }
}
