
$fn = 64;

module inverse() difference() { square(1e6,center=true); child(); }
module offset(d=1) minkowski() { circle(d); child(); }

module s() {
	inverse() offset(2) inverse() offset(2) union() { 
		hull() { circle(r=4); translate([0,3]) circle(r=4); } 
		translate([0,-3]) hull() { circle(r=1); translate([10,0]) circle(r=1); } 
	}
}
difference() {
	linear_extrude(height=3,center=true,convexity=3) s();
	linear_extrude(height=9,center=true) circle(r=2);
}
