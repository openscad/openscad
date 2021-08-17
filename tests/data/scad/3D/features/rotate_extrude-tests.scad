// Empty
rotate_extrude();
// No children
rotate_extrude() { }
// 3D child
rotate_extrude() { cube(); }

// Normal
rotate_extrude() translate([20,0,0]) circle(r=10);

// Sweep of polygon with hole
translate([50,-20,0]) {
  difference() { 
    rotate_extrude(convexity=4) translate([20,0,0]) difference() {
      circle(r=10); circle(r=8);
    }
    translate([-50,0,0]) cube([100,100,100], center=true);
  }
}

// Alternative, difference between two solid sweeps
translate([50,50,0]) {
  difference() { 
    difference() {
      rotate_extrude(convexity=2) translate([20,0,0]) circle(r=10);
      rotate_extrude(convexity=2) translate([20,0,0]) circle(r=8);
    }
    translate([-50,0,0]) cube([100,100,100], center=true);
  }
}

// Minimal $fn
translate([0,-60,0]) rotate_extrude($fn=1) translate([20,0,0]) circle(r=10,$fn=1);

// Object in negative X
translate([0,60,0]) rotate_extrude() translate([-20,0]) square(10);

