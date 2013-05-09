// Empty
rotate_extrude();
// No children
rotate_extrude() { }
// 3D child
rotate_extrude() { cube(); }

linear_extrude(height=10) square([10,10]);
translate([19,5,0]) linear_extrude(height=10, center=true) difference() {circle(5); circle(3);}
translate([31.5,2.5,0]) linear_extrude(height=10, twist=-45) polygon(points = [[-5,-2.5], [5,-2.5], [0,2.5]]);

translate([0,20,0]) linear_extrude(height=20, twist=45, slices=2) square([10,10]);
translate([19,20,0]) linear_extrude(height=20, twist=45, slices=10) square([10,10]);

translate([-15,0,0]) linear_extrude(5) square([10,10]);
