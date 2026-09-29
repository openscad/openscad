// A deliberately long model for the depth map, staggered so that depth is
// actually visible end-on.
//
// Depth is normalized across the model's bounding sphere, capped at the viewing
// distance, so the range is the same from every direction. These two views exist
// to hold that: end-on and side-on must grade against one range, not two. Before
// that change the range was the bounding box's extent along the view axis - 200
// end-on against ~50 side-on here - and the shading visibly rebalanced as the
// model turned (reported from dogfooding on "extruder illustration", measured
// there as a 3.3x swing in mean brightness across three views).
//
// The side view is deliberately the unflattering one: an orientation-invariant
// range costs contrast exactly where the model is thin in depth, and that cost
// should be visible in the suite rather than only in the argument for it.
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
