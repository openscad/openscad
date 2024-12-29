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

module polygonWithOverlappingHole() {
fill() polygon(
  points=[
    [0,0], [20,0], [20,20], [0,20],
    [10,10], [30,10], [30,30], [10,30],
  ],
  paths=[
    [0,1,2,3],
    [4,5,6,7],
  ]
);
}

translate([-40,0,0]) convex2dSimple();
convex2dHole();
translate([40,04,0]) enclaves();
translate([0,-20,0]) someLetters();
fill2null();
multiHole();
translate([30,20,0]) polygonWithOverlappingHole();
