// .dat file has values from 2-9
scale([1,1,0.2]) surface("issue1223.dat");
translate([0,-1,-0.1]) cube([2,1,0.1]);
translate([-1,0,-0.1]) cube([1,2,0.1]);

// .dat file has incomplete last line
translate([4,0,0]) {
    scale([1,1,0.2]) surface("issue1223-2.dat");
    translate([0,-1,-0.1]) cube([2,1,0.1]);
    translate([-1,0,-0.1]) cube([1,3,0.1]);
}
