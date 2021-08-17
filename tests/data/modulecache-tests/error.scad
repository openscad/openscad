//error.scad
a=10   // syntax error
b=2;

module errmod() { cube(a); }

errmod();
