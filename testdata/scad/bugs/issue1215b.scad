module dieShape(out) {
  rotate([-90,90,0]) cylinder(r=10,h=15+out, $fn=4);
  rotate([0,-90,0]) cylinder(r=10,h=15+out, $fn=4);
  rotate([0,0,0]) cylinder(r=10,h=15+out, $fn=4);
}
    
hull() dieShape(0);
dieShape(.5);
