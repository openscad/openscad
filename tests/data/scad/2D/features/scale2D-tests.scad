module obj2D() square([2,3], center=true);

// 3 variants of 3D scale of 2D object
scale([2,4/3,2]) obj2D();
translate([5,0,0]) scale([2,4/3]) obj2D();
translate([10,0,0]) scale(2) obj2D();

// Scale by zero; 2D object
translate([-5,0,0]) linear_extrude() scale([0,0]) obj2D();
translate([-5,0,0]) linear_extrude() scale([0,1]) obj2D();
