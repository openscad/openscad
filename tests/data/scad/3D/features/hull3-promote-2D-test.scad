// Test that mixing 2D and 3D objects in hull will promote the
// 2D object to 3D and then properly perform the hull operation.
translate([-20, 20, 0]) hull() {
    square([15, 15], center=true);                          // 2D
    cylinder(d=10, h=10);                                   // 3D
}

// This promotion should hold regardless of ordering, so test that.
translate([20, 20, 0]) hull() {
    translate([0, 0, 7.5]) cube([10, 10, 10], center=true); // 3D
    circle(r=10);                                           // 2D
}

// More than two objects should still work, so test that.
translate([-20, -20, 0]) hull() {
    square([15, 15], center=true);                          // 2D
    cylinder(d=10, h=10);                                   // 3D
    rotate([0, 0, 45]) square([15, 15], center=true);       // 2D
}

// Ordering does not matter even with more than two objects, test that.
translate([20, -20, 0]) hull() {
    cylinder(d=10, h=10);                                   // 3D
    square([15, 15], center=true);                          // 2D
    rotate([0, 0, 45]) square([15, 15], center=true);       // 2D
}
