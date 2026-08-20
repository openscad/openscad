// A deliberately long model for the depth map, staggered so that depth is
// actually visible end-on.
//
// The viewport and export normalize depth across the bounding box's extent along
// the current view axis, which is orientation dependent: this model's extent is
// 200 seen end-on and ~50 seen side-on, so the same geometry grades over a very
// different range in the two views. Reported from dogfooding on "extruder
// illustration", which has this shape - measured there as a 3.3x swing in mean
// brightness across three views.
//
// The rungs are offset laterally on purpose. An earlier version of this file put
// everything on the axis, and end-on the nearest element simply occluded all the
// rest - a view that looks alarming (one bright blob, everything else black) but
// says nothing about depth encoding, because there was no farther surface left
// visible to encode.
for (i = [0:9]) {
  translate([i * 20 - 90, sin(i * 72) * 18, cos(i * 72) * 18])
    cube([14, 10, 10], center = true);
}
cube([200, 5, 5], center = true);
