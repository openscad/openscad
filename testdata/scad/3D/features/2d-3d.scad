// Test a mix of toplevel 2D and 3D objects
cube();
translate([2,0,0]) square();

// Test mixing of empty 2D and 3D objects
union() {
    cube(0);
    circle(0);
}
