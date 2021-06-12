$fn=16;

module parent(range=[0:2]) {
  for (i=range) {
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

// Leaking variables to child list is not allowed
translate([0,6,0]) parent(range=[0:1], testvar=10) { sphere(); cube(testvar, center=true);}
