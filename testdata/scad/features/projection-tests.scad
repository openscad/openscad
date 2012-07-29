// Empty
projection();
// No children
projection() { }
// 2D child
projection(cut=true) { square(); }

linear_extrude(height=20) projection(cut=false) sphere(r=10);
translate([22,0,0]) linear_extrude(height=20) projection(cut=true) translate([0,0,9]) sphere(r=10);
translate([44,0,0]) linear_extrude(height=20) projection(cut=true) translate([0,0,7]) sphere(r=10);

// Boundary case: clipping the top of a cube
translate([0,-22,0]) linear_extrude(height=5) projection(cut=true) translate([0,0,-4.999999]) cube(10, center=true);

// holes
translate([0,-44,0]) linear_extrude(height=5) projection(cut=true) 
	union() {
		difference() { cube(5,center=true); cube(4,center=true); }
		translate([2.1,2.1]) difference() { cube(5,center=true); cube(4,center=true); }
	}
