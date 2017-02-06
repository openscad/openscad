//===========================================
//   Public Domain Epi- and Hypo- trochoids in OpenSCAD
//   version 1.0
//   by Matt Moses, 2011, mmoses152@gmail.com
//   http://www.thingiverse.com/thing:8067
//
//   This file is public domain.  Use it for any purpose, including commercial
//   applications.  Attribution would be nice, but is not required.  There is
//   no warranty of any kind, including its correctness, usefulness, or safety.
//
//   An EPITROCHOID is a curve traced by a point 
//   fixed at a distance "d" 
//   to the center of a circle of radius "r"
//   as the circle rolls 
//   outside another circle of radius "R".
//
//   An HYPOTROCHOID is a curve traced by a point 
//   fixed at a distance "d" 
//   to the center of a circle of radius "r"
//   as the circle rolls 
//   inside another circle of radius "R".
//
//   An EPICYCLOID is an epitrochoid with d = r.
//
//   An HYPOCYCLOID is an hypotrochoid with d = r.
//
//   See http://en.wikipedia.org/wiki/Epitrochoid
//   and http://en.wikipedia.org/wiki/Hypotrochoid
// 
//   Beware the polar forms of the equations on Wikipedia...
//   They are correct, but theta is measured to the center of the small disk!!
//===========================================

// There are several different methods for extruding.  The best are probably
// the ones using linear extrude.


//===========================================
//   Demo - draws one of each, plus some little wheels and sticks.
//
//   Fun stuff to try: 
//   Animate, try FPS = 5 and Steps = 200
//   R = 2, r = 1, d = 0.2
//   R = 4, r = 1, d = 1
//   R = 2, r = 1, d = 0.5
//   
//   What happens when you make d > r ??
//   What happens when d < 0 ??
//   What happens when r < 0 ??
//
//===========================================

$fn = 30;

thickness = 2;
R = 4;
r = 1;
d = 1;
n = 60; // number of wedge segments

alpha = 360*$t;

color([0, 0, 1])
translate([0, 0, -0.5])
	cylinder(h = 1, r= R, center = true);

color([0, 1, 0])
epitrochoid(R,r,d,n,thickness);

color([1, 0, 0])
translate([ (R+r)*cos(alpha) , (R+r)*sin(alpha), -0.5]) {
	rotate([0, 0, alpha + R/r*alpha]) {
		cylinder(h = 1, r = r, center = true);
		translate([-d, 0, 1.5]) {
			cylinder(h = 2.2, r = 0.1, center = true);
		}
	}
}


translate([2*(abs(R) + abs(r) + abs(d)), 0, 0]){
color([0, 0, 1])
translate([0, 0, -0.5])
	difference() {
		cylinder(h = 1, r = 1.1*R, center = true);
		cylinder(h = 1.1, r= R, center = true);
	}

color([0, 1, 0])
hypotrochoid(R,r,d,n,thickness);

color([1, 0, 0])
translate([ (R-r)*cos(alpha) , (R-r)*sin(alpha), -0.5]) {
	rotate([0, 0, alpha - R/r*alpha]) {
		cylinder(h = 1, r = r, center = true);
		translate([d, 0, 1.5]) {
			cylinder(h = 2.2, r = 0.1, center = true);
		}
	}
}
}

// This just makes a twisted hypotrochoid
translate([0,14, 0])
hypotrochoidLinear(4, 1, 1, 40, 40, 10, 30);

//   End of Demo Section
//===========================================


//===========================================
// Epitrochoid
//
module epitrochoid(R, r, d, n, thickness) {
	dth = 360/n;
	for ( i = [0:n-1] ) {
			polyhedron(points = [[0,0,0], 
			[(R+r)*cos(dth*i) - d*cos((R+r)/r*dth*i), (R+r)*sin(dth*i) - d*sin((R+r)/r*dth*i), 0], 
			[(R+r)*cos(dth*(i+1)) - d*cos((R+r)/r*dth*(i+1)), (R+r)*sin(dth*(i+1)) - d*sin((R+r)/r*dth*(i+1)), 0], 
			[0,0,thickness], 
			[(R+r)*cos(dth*i) - d*cos((R+r)/r*dth*i), (R+r)*sin(dth*i) - d*sin((R+r)/r*dth*i), thickness], 
			[(R+r)*cos(dth*(i+1)) - d*cos((R+r)/r*dth*(i+1)), (R+r)*sin(dth*(i+1)) - d*sin((R+r)/r*dth*(i+1)), thickness]],
			triangles = [[0, 2, 1], 
			[0, 1,  3], 
			[3, 1, 4], 
			[3, 4, 5], 
			[0, 3, 2], 
			[2, 3, 5],
			[1, 2, 4],
			[2, 5, 4]]);
	}
}
//===========================================


//===========================================
// Hypotrochoid
//
module hypotrochoid(R, r, d, n, thickness) {
	dth = 360/n;
	for ( i = [0:n-1] ) {
			polyhedron(points = [[0,0,0], 
			[(R-r)*cos(dth*i) + d*cos((R-r)/r*dth*i), (R-r)*sin(dth*i) - d*sin((R-r)/r*dth*i), 0], 
			[(R-r)*cos(dth*(i+1)) + d*cos((R-r)/r*dth*(i+1)), (R-r)*sin(dth*(i+1)) - d*sin((R-r)/r*dth*(i+1)), 0], 
			[0,0,thickness], 
			[(R-r)*cos(dth*i) + d*cos((R-r)/r*dth*i), (R-r)*sin(dth*i) - d*sin((R-r)/r*dth*i), thickness], 
			[(R-r)*cos(dth*(i+1)) + d*cos((R-r)/r*dth*(i+1)), (R-r)*sin(dth*(i+1)) - d*sin((R-r)/r*dth*(i+1)), thickness]],
			triangles = [[0, 2, 1], 
			[0, 1,  3], 
			[3, 1, 4], 
			[3, 4, 5], 
			[0, 3, 2], 
			[2, 3, 5],
			[1, 2, 4],
			[2, 5, 4]]);
	}
}
//===========================================


//===========================================
// Epitrochoid Wedge with Bore
//
module epitrochoidWBore(R, r, d, n, p, thickness, rb) {
	dth = 360/n;
	union() {
	for ( i = [0:p-1] ) {
			polyhedron(points = [[rb*cos(dth*i), rb*sin(dth*i),0], 
			[(R+r)*cos(dth*i) - d*cos((R+r)/r*dth*i), (R+r)*sin(dth*i) - d*sin((R+r)/r*dth*i), 0], 
			[(R+r)*cos(dth*(i+1)) - d*cos((R+r)/r*dth*(i+1)), (R+r)*sin(dth*(i+1)) - d*sin((R+r)/r*dth*(i+1)), 0], 
			[rb*cos(dth*(i+1)), rb*sin(dth*(i+1)), 0], 
			[rb*cos(dth*i), rb*sin(dth*i), thickness],
			[(R+r)*cos(dth*i) - d*cos((R+r)/r*dth*i), (R+r)*sin(dth*i) - d*sin((R+r)/r*dth*i), thickness], 
			[(R+r)*cos(dth*(i+1)) - d*cos((R+r)/r*dth*(i+1)), (R+r)*sin(dth*(i+1)) - d*sin((R+r)/r*dth*(i+1)), thickness],
			[rb*cos(dth*(i+1)), rb*sin(dth*(i+1)), thickness]],
			triangles = [[0, 1, 4], [4, 1, 5],
			[1, 2, 5], [5, 2, 6], 
			[2, 3, 7], [7, 6, 2], 
			[3, 0, 4], [4, 7, 3], 
			[4, 5, 7], [7, 5, 6], 
			[0, 3, 1], [1, 3, 2]]); 
	}
	}
}
//===========================================


//===========================================
// Epitrochoid Wedge with Bore, Linear Extrude
//
module epitrochoidWBoreLinear(R, r, d, n, p, thickness, rb, twist) {
	dth = 360/n;
	linear_extrude(height = thickness, convexity = 10, twist = twist) {
	union() {
	for ( i = [0:p-1] ) {
			polygon(points = [[rb*cos(dth*i), rb*sin(dth*i)], 
			[(R+r)*cos(dth*i) - d*cos((R+r)/r*dth*i), (R+r)*sin(dth*i) - d*sin((R+r)/r*dth*i)], 
			[(R+r)*cos(dth*(i+1)) - d*cos((R+r)/r*dth*(i+1)), (R+r)*sin(dth*(i+1)) - d*sin((R+r)/r*dth*(i+1))], 
			[rb*cos(dth*(i+1)), rb*sin(dth*(i+1))]],
			paths = [[0, 1, 2, 3]], convexity = 10); 
	}
	}
	}
}
//===========================================


//===========================================
// Epitrochoid Wedge, Linear Extrude
//
module epitrochoidLinear(R, r, d, n, p, thickness, twist) {
	dth = 360/n;
	linear_extrude(height = thickness, convexity = 10, twist = twist) {
	union() {
	for ( i = [0:p-1] ) {
			polygon(points = [[0, 0], 
			[(R+r)*cos(dth*i) - d*cos((R+r)/r*dth*i), (R+r)*sin(dth*i) - d*sin((R+r)/r*dth*i)], 
			[(R+r)*cos(dth*(i+1)) - d*cos((R+r)/r*dth*(i+1)), (R+r)*sin(dth*(i+1)) - d*sin((R+r)/r*dth*(i+1))]], 
			paths = [[0, 1, 2]], convexity = 10); 
	}
	}
	}
}
//===========================================


//===========================================
// Hypotrochoid Wedge with Bore
//
module hypotrochoidWBore(R, r, d, n, p, thickness, rb) {
	dth = 360/n;
	union() {
	for ( i = [0:p-1] ) {
			polyhedron(points = [[rb*cos(dth*i), rb*sin(dth*i),0], 
			[(R-r)*cos(dth*i) + d*cos((R-r)/r*dth*i), (R-r)*sin(dth*i) - d*sin((R-r)/r*dth*i), 0], 
			[(R-r)*cos(dth*(i+1)) + d*cos((R-r)/r*dth*(i+1)), (R-r)*sin(dth*(i+1)) - d*sin((R-r)/r*dth*(i+1)), 0], 
			[rb*cos(dth*(i+1)), rb*sin(dth*(i+1)), 0], 
			[rb*cos(dth*i), rb*sin(dth*i), thickness],
			[(R-r)*cos(dth*i) + d*cos((R-r)/r*dth*i), (R-r)*sin(dth*i) - d*sin((R-r)/r*dth*i), thickness], 
			[(R-r)*cos(dth*(i+1)) + d*cos((R-r)/r*dth*(i+1)), (R-r)*sin(dth*(i+1)) - d*sin((R-r)/r*dth*(i+1)), thickness],
			[rb*cos(dth*(i+1)), rb*sin(dth*(i+1)), thickness]],
			triangles = [[0, 1, 4], [4, 1, 5],
			[1, 2, 5], [5, 2, 6], 
			[2, 3, 7], [7, 6, 2], 
			[3, 0, 4], [4, 7, 3], 
			[4, 5, 7], [7, 5, 6], 
			[0, 3, 1], [1, 3, 2]]); 
	}
	}
}
//===========================================


//===========================================
// Hypotrochoid Wedge with Bore, Linear Extrude
//
module hypotrochoidWBoreLinear(R, r, d, n, p, thickness, rb, twist) {
	dth = 360/n;
	linear_extrude(height = thickness, convexity = 10, twist = twist) {
	union() {
	for ( i = [0:p-1] ) {
			polygon(points = [[rb*cos(dth*i), rb*sin(dth*i)], 
			[(R-r)*cos(dth*i) + d*cos((R-r)/r*dth*i), (R-r)*sin(dth*i) - d*sin((R-r)/r*dth*i)], 
			[(R-r)*cos(dth*(i+1)) + d*cos((R-r)/r*dth*(i+1)), (R-r)*sin(dth*(i+1)) - d*sin((R-r)/r*dth*(i+1))], 
			[rb*cos(dth*(i+1)), rb*sin(dth*(i+1))]], 
			paths = [[0, 1, 2, 3]], convexity = 10); 
	}
	}
	}
}
//===========================================


//===========================================
// Hypotrochoid Wedge, Linear Extrude
//
module hypotrochoidLinear(R, r, d, n, p, thickness, twist) {
	dth = 360/n;
	linear_extrude(height = thickness, convexity = 10, twist = twist) {
	union() {
	for ( i = [0:p-1] ) {
			polygon(points = [[0, 0], 
			[(R-r)*cos(dth*i) + d*cos((R-r)/r*dth*i), (R-r)*sin(dth*i) - d*sin((R-r)/r*dth*i)], 
			[(R-r)*cos(dth*(i+1)) + d*cos((R-r)/r*dth*(i+1)), (R-r)*sin(dth*(i+1)) - d*sin((R-r)/r*dth*(i+1))]],
			paths = [[0, 1, 2]], convexity = 10); 
	}
	}
	}
}
//=========================================== 
