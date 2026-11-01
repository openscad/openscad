// Test various scenarios for a function that tells you
// how many sides a circular object would have.

// These tests do not attempt to test all possible combinations
// of the $fx variables.  We assume that the $ variable mechanisms
// and the $fx mechanisms are basically working, and we test
// that the function produces the same results as circle(),
// showing that the argument processing and plumbing to the shared
// circle-resolution algorithm is working.

// Set the global parameters for a really small number of sides,
// so that it's easy to look at a picture and confirm that it's
// the right number.
$fa = 45;   // 8 sides
$fs = 5;    // Too small to affect an octagon at this radius

// All of these use the same size circle, sometimes expressed
// as radius and sometimes as diameter.
r = 20;

// Each of these echos the result for that scenario, and then
// in the lower row creates a circle using circle() and letting
// it figure out how many sides to display, then in the upper
// row creates a circle explicitly telling it to use the number
// calculated by the function.

// Pick up the default parameters (which are set above), specifying
// the size using the radius.
echo(fn(r=r));
translate([0*r*3,  0]) circle(r=r);
translate([0*r*3, r*3]) circle(r=r, $fn=fn(r=r));

// Default parameters, specifying the size using the diameter.
echo(fn(d=2*r));
translate([1*r*3,  0]) circle(d=2*r);
translate([1*r*3, r*3]) circle(d=2*r, $fn=fn(d=2*r));

// Default parameters, specifying both radius and diameter - diameter should win,
// and there should be four warning messages - one for each circle call, and one
// for each function call.
echo(fn(r=r/2,d=2*r));
translate([2*r*3,  0]) circle(r=r/2, d=2*r);
translate([2*r*3, r*3]) circle(r=r/2, d=2*r, $fn=fn(r=r/2, d=2*r));

// Explicitly specify seven sides.
echo(fn(r=r, $fn=7));
translate([3*r*3,  0]) circle(r=r, $fn=7);
translate([3*r*3, r*3]) circle(r=r, $fn=fn(r=r, $fn=7));

// Set $fs/$fa for this one set of calls, to generate a relatively fine
// circle.  Note that the dump tests also show that the $fx settings
// used for the function call are not somehow leaking into the circle()
// call.
echo(fn(r=r, $fa=1, $fs=1));
translate([4*r*3,  0]) circle(r=r, $fa=1, $fs=1);
translate([4*r*3, r*3]) circle(r=r, $fn=fn(r=r, $fa=1, $fs=1));
