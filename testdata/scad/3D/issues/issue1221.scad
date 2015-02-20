$fn=4;
difference(){
!    union(){
        translate([0,0,-1.5]) cylinder(h=9, r=26, center=true);
        translate([0.0, 0.0, 6.0]) rotate([180, 0, 0]) cylinder(h=3, r1=25, r2=26);
    }
    cylinder(h=14, r=23.0, center=true);
}
