// Test special cases of linear_extrude with twist.
// When an edge has one vertex on the origin, the diagonals will be equal, or very nearly so (floating point inaccuracy).
// Extra logic is required to determine diagonal choice in these cases.

// Test all kinds of combinations of:
//   * positive/negative twist
//   * positive/negative geometry
//   * mirror([x,y]) and mirror([0,0,z])
//   * circle() vs polygon(dir)   both winding orders tested with polygon

R = 3;
slices = 1;
twist = 90;
$fn=4;
do_projection = true; // Set to false to see 3D view without projection, for manual diagnosis purposes.
use_cut = true;       // projection cut option

// === Calculated Params ===
// Profile is shifted before extrusion. So that one vertex is on the origin
shift2d = [-R,0];
// Total extrusion height, for square slice quads
h = slices*(2*PI*R)/$fn;
// XY spacing between extrusions
sp = (R+1)*3;
// Points to test if polygon(), and winding order has any effect vs builtin circle()
pts = [ for(i = [0:1:$fn-1]) (R+1) * [cos(i*360/$fn), sin(i*360/$fn)],
        for(i = [0:1:$fn-1])  R    * [cos(i*360/$fn), sin(i*360/$fn)] ];
// Path of smaller "circle"
paths = [[for(i=[0:1:$fn-1]) i+$fn]];
// Path of reversed-order circle
rpaths = [[for(i=[$fn-1:-1:0]) i+$fn]];
// Paths for hollow circle
negpaths = [[for(i=[0:1:$fn-1]) i], paths[0]];
// Paths for reversed-order hollow circle
rnegpaths = [[for(i=[$fn-1:-1:0]) i], rpaths[0]];

module test(twist, m, zoff) {
  if (do_projection) {
    projection(cut=use_cut) translate([0,0,-h/2])
      linear_extrude(height=h, twist=twist, slices=slices) translate(shift2d) mirror(m) children(0);
  } else {
    translate([0,0,zoff])
      linear_extrude(height=h, twist=twist, slices=slices) translate(shift2d) mirror(m) children(0);
  }
}

// outer loop over positive/negative twist.  Negative is translated below Z axis
for(s=[1,-1]) let(a=s*twist, zoff=s==1?0:-h) {
  // Loop over mirror([x,y])
  for(mx=[0,1],my=[0,1]) {
    off = sp*[mx, my];
    // Positive geometry
    translate(sp*[0,0]+off) test(a, [mx,my], zoff) circle(R);
    translate(sp*[0,2]+off) test(a, [mx,my], zoff) polygon(pts, paths);
    translate(sp*[0,4]+off) test(a, [mx,my], zoff) polygon(pts, rpaths);
    // Negative geometry
    translate(sp*[4,0]+off) test(a, [mx,my], zoff) difference() { circle(R+1); circle(R); }
    translate(sp*[4,2]+off) test(a, [mx,my], zoff) polygon(pts, negpaths);
    translate(sp*[4,4]+off) test(a, [mx,my], zoff) polygon(pts, rnegpaths);
  }
  // Loop over mirror([0,0,z])
  for(mz=[0,1]) {
    pos = sp*[0, mz];
    // Positive geometry
    translate(sp*[2,0]+pos) test(a, [0,0,mz], zoff) circle(R);
    translate(sp*[2,2]+pos) test(a, [0,0,mz], zoff) polygon(pts, paths);
    translate(sp*[2,4]+pos) test(a, [0,0,mz], zoff) polygon(pts, rpaths);
    neg = sp*[1, mz];
    // Negative geometry
    translate(sp*[2,0]+neg) test(a, [0,0,mz], zoff) difference() { circle(R+1); circle(R); }
    translate(sp*[2,2]+neg) test(a, [0,0,mz], zoff) polygon(pts, negpaths);
    translate(sp*[2,4]+neg) test(a, [0,0,mz], zoff) polygon(pts, rnegpaths);
  }
}
