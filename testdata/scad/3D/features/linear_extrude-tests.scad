// Empty
rotate_extrude();
// No children
rotate_extrude() { }
// 3D child
rotate_extrude() { cube(); }

linear_extrude(height=10) square([10,10]);
translate([19,5,0]) linear_extrude(height=10, center=true) difference() {circle(5); circle(3);}
translate([31.5,2.5,0]) linear_extrude(height=10, twist=-45) polygon(points = [[-5,-2.5], [5,-2.5], [0,2.5]]);

translate([0,20,0]) linear_extrude(height=20, twist=30, slices=2) {
    difference() {
        square([10,10]);
        translate([1,1]) square([8,8]);
    }
}
translate([19,20,0]) linear_extrude(height=20, twist=45, slices=10) square([10,10]);

translate([0,-15,0]) linear_extrude(5) square([10,10]);

// scale given as a scalar
translate([-25,-10,0]) linear_extrude(height=10, scale=2) square(5, center=true);
// scale given as a 3-dim vector
translate([-15,20,0]) linear_extrude(height=20, scale=[4,5,6]) square(10);
// scale is negative
translate([-10,5,0]) linear_extrude(height=15, scale=-2) square(10, center=true);
// scale given as undefined
translate([-15,-15,0]) linear_extrude(height=10, scale=var_undef) square(10);

// height is negative
translate([0,-25,0]) linear_extrude(-1) square(10, center=true);
