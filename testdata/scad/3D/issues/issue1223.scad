// .dat file has values from 2-9
scale([1,1,0.2]) surface("issue1223.dat");
translate([1,-1,0]) cube([1,1,0.1]);
translate([-1,1,0]) cube([1,1,0.1]);

// .dat file has incomplete last line
translate([4,0,0]) {
    scale([1,1,0.2]) surface("issue1223-2.dat");
    translate([1,-1,0]) cube([1,1,0.1]);
    translate([-1,1,0]) cube([1,2,0.1]);
}
