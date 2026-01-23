// This file is to test features which can't be seen (well or at-all) without edges enabled.

// Non-uniformly-scaled untwisted extrude, default slicing.
// Expect multiple horizontal slices.
linear_extrude(h=30, scale=[1/20,20]) scale([1,0.1]) rotate(45) square(10, center=true);

// Uniformly-scaled untwisted extrude, default slicing.
// Expect no horizontal slices.
translate([-20,0,0]) linear_extrude(h=30, scale=[10,10]) scale([0.1,0.1]) rotate(45) square(10, center=true);

// Uniformly-scaled untwisted extrude, explicit slicing.
// Expect three horizontal slices (2 cuts).
translate([-40,0,0]) linear_extrude(h=30, scale=[10,10], slices=3) scale([0.1,0.1]) rotate(45) square(10, center=true);
