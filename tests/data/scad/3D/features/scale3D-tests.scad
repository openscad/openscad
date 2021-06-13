module obj3D() cylinder(r=1, center=true, $fn=8);

// 3 variants of 3D scale of 3D object
translate([0,0,0]) scale([1,2,3]) obj3D();
translate([5,0,0]) scale([2,1]) obj3D();
translate([10,0,0]) scale(2) obj3D();

// Scale by zero; 3D object
linear_extrude() scale([0,0,0]) obj3D();
linear_extrude() scale([0,1,0]) obj3D();
linear_extrude() scale([1,1,0]) obj3D();
