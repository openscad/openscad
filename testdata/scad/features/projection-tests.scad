// Empty
projection();
// No children
projection() { }
// 2D child
projection() { square(); }

// Simple
projection(cut=false) cube(10);

// Two children
translate([-12,0]) projection(cut=false) {
    cube(10);
    difference() {
      sphere(10);
      cylinder(h=30, r=5, center=true);
    }
}

// Holes
translate([6,-12]) projection(cut=false) {
    cube(10);
    difference() {
      sphere(10);
      cylinder(h=30, r=5, center=true);
    }
}
