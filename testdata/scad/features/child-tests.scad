$fn=16;

module parent() {
  for (i=[0:2]) {
    translate([2.5*i,0,0]) child(i);
  }
}

// Normal
parent() {
  sphere();
  cylinder(h=2, center=true);
  cube(2, center=true);
}

// No children
parent();

// Too few children
translate([0,3,0]) parent() { sphere(); }

// No parameter to child
module parent2() {
  child();
}

translate([2.5,3,0]) parent2() { cylinder(h=2, center=true); sphere(); }

// Negative parameter to child
module parent3() {
  child(-1);
}

translate([5,3,0]) parent3() { cube(); sphere(); }
