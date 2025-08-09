// Test that mixing 2D and 3D objects in minkowski will promote the
// 2D object to 3D and then properly perform the minkowski sum.
translate([-20, 0, 0]) minkowski() {
    square([15, 15], center=true);                          // 2D
    cylinder(d=10, h=10);                                   // 3D
}

// This promotion should hold regardless of ordering, so test that.
translate([20, 0, 0]) minkowski() {
    cube([15, 15, 15], center=true);                        // 3D
    circle(d=10);                                           // 2D
}
