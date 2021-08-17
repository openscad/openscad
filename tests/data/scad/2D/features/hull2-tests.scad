module convex2dSimple() {
    hull() {
        translate([15,10]) circle(10);
        circle(10);
    }
}

module concave2dSimple() {
    hull() {
        translate([15,10]) square(2);
        translate([15,0]) square(2);
        square(2);
    }
}

module convex2dHole() {
    hull() {
        translate([15,10,0]) circle(10);
        difference() {
            circle(10);
            circle(5);
        }
    }
}

module hull2dForLoop() {
  hull() {
    for(x = [0,10])
      for(y=[0,10])
        translate([x,y]) circle(3);
  }
}

module hull2null() {
  hull() {
    square(0);
    circle(0);
  }
}

convex2dHole();
translate([40,0,0]) convex2dSimple();
translate([0,-20,0]) concave2dSimple();
translate([30,-25,0]) hull2dForLoop();
hull2null();