$fn=32;

intersection(){
    difference(){
        cube(size=50, center=true);
        cylinder(d=30,h=80, center=true);
    }
    cylinder(d=65,h=80,center=true);
    rotate([90,0,0]) cylinder(d=65,h=80,center=true);
}
