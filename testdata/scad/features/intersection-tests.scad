// Empty
intersection();

// No children
intersection() { }

intersection() {
  sphere(r=5);
  translate([0,0,3]) cube([4,4,6], center=true);
}

translate([0,12,0]) intersection() {
  cube([10,10,10], center=true);
  cylinder(r=4, h=12, center=true);
}

translate([12,0,0]) intersection() {
  cube([10,10,10], center=true);
  cylinder(r=4, h=12, center=true);
  rotate([0,90,0]) cylinder(r=4, h=12, center=true);
}

translate([12,12,0]) intersection() {
  cube([10,10,10], center=true);
  translate([0,0,7]) cylinder(r=4, h=4, center=true);
}

translate([24,0,0]) intersection() {
  cube([10,10,10], center=true);
  translate([0,0,6.99]) cylinder(r=4, h=4, center=true);
}

translate([-12,0,0]) intersection() {
  cube([10,10,10], center=true);
  translate([0,-10,-10]) cube([10,10,10], center=true);
}

translate([-12,12,0]) intersection() {
  cube([10,10,10], center=true);
  translate([0,-9.99,-9.99]) cube([10,10,10], center=true);
}

translate([-12,0,0]) intersection() {
 scale(10/sqrt(2)) 
  polyhedron(points = [[1,0,0],[-1,0,0],[0,1,0],[0,-1,0],[0,0,1],[0,0,-1]],
	     triangles = [[0,2,4],[0,5,2],[0,4,3],[0,3,5],[1,4,2],[1,2,5],[1,3,4], [1,5,3]],
	     convexity = 1);
 translate([0,0,10]) cube([20,20,20], center=true);
}
