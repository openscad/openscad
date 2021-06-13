$fn = 3; // don't split triangles into segments

// linear_extrude tests with scale to 0 on different axes. 
// Test updated with additional cases for PR #3351 - Hans Loeblich
module tri() { polygon([[20, 0],[0, 10],[0, -10]]); }

// Original test had the following single geometry:
translate([ 25,-20]) linear_extrude(height = 10, scale = [1,0], slices = 2) tri();
// Newly added cases:
translate([ 25, 20]) linear_extrude(height = 10, scale = [1,0], slices = 1) tri();
translate([  0,-20]) linear_extrude(height = 10, scale = [0,0], slices = 2) tri();
translate([  0, 20]) linear_extrude(height = 10, scale = [0,0], slices = 1) tri();  

// FIXME: the following two cases preview ok, but render with non-manifold warning.
// TEST_GENERATE only makes blank images for monotonepngtest and stlcgalpngtest
// while the "actual" images contain all objects, so those tests fail.
// I think export/import is involved in generating those expected images and that's failing?
/*
translate([-25,-20]) linear_extrude(height = 10, scale = [0,1], slices = 2) tri();
translate([-25, 20]) linear_extrude(height = 10, scale = [0,1], slices = 1) tri();
//*/
