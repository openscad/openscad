scale([1,1,1]) surface("issue1223b.png");
translate([1,-1,0]) cube([2,1,0.1]);
translate([-1,1,0]) cube([1,2,0.1]);

translate([5,0,0]) {
    surface("issue1223b.dat");
    translate([1,-1,0]) cube([2,1,0.1]);
    translate([-1,1,0]) cube([1,2,0.1]);
}