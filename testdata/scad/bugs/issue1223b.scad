scale([1,1,0.02]) surface("issue1223b.png");
translate([0,-1,-0.1]) cube([3,1,0.1]);
translate([-1,0,-0.1]) cube([1,3,0.1]);

translate([5,0,0]) {
    surface("issue1223b.dat");
    translate([0,-1,-0.1]) cube([3,1,0.1]);
    translate([-1,0,-0.1]) cube([1,3,0.1]);
}