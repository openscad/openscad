// Test precision issues during transformations

// test Resize --> resize has an internal transformation. this should be empty
difference() {
  resize([10,10,10]) cube([5,40,43]);
  cube([10,10,10]);
}

// Test rotate(), shoud be blank, but it isn't
translate([20,0]) 
difference() {
  rotate([360,0,0]) cube([10,10,10]);
  cube([10,10,10]);
}

// have something for the image for reference
cube();
