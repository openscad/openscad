module obj2D() polygon([[-0.5,-0.5], [1,-0.5], [1,1], [-0.5, 0.5]])
square([2,3], center=true);

linear_extrude(1) scale([1, -1]) obj2D();
translate([3,0,0]) linear_extrude(1) scale([-1, -0.5]) obj2D();
translate([0,3,0]) linear_extrude(1) mirror() obj2D();
translate([2,3,0]) linear_extrude(1) mirror([0,1]) obj2D();
