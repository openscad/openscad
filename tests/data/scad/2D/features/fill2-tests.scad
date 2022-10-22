module convex2dSimple() {
    fill() {
        translate([15,10]) circle(10);
        circle(10);
    }
}

module convex2dHole() {
    fill() {
        translate([15,10,0]) circle(10);
        difference() {
            circle(10);
            circle(5);
        }
    }
}

module someLetters() {
   fill() {
       text("Qaeio%&");
   }
}

module fill2null() {
  fill() {
    square(0);
    circle(0);
  }
}

module multiHole() {
  fill() difference() {
   union() {
     translate([0,25]) square([15,1]);
     translate([0,25]) square([1,15]);
     translate([18,34]) circle(10);
     polygon(points=[[0,40],[5,45],[15,40]]);
   }
   translate([20,34]) circle(9);
   }
}

module enclaves() {
  fill() {
      difference() {
        circle(10);
        difference() {
          circle(8);
          difference() {
            circle(6);
            circle(4);
          }
        }
      }
  }
}
  
convex2dHole();
translate([40,04,0]) enclaves();
translate([0,-20,0]) someLetters();
fill2null();
multiHole();