svg_circle = "../../svg/svg-spec/spec-shapes-circle01.svg";
svg_sel = "../../svg/id-layer-selection-test.svg";
svg_simple = "../../svg/simple.svg";

// Extracts the top cap of a centered r=10 shape (y in [9.45, 10.0]) and
// stretches Y by 7x so individual tessellation facets are large and countable.
module cap_slice() {
  scale([1, 7]) intersection() {
    translate([0, -9.45]) children();
    translate([0, 0.3]) square([7, 0.7], center=true);
  }
}

// Column 1: <circle> (r=10mm, minimum 40 segments -> 4 facets in cap)
// Row 0: default (40 segs), Row 1: $fn=60, Row 2: $fa=3 (120 segs), Row 3: $fs=8 (79 segs)
translate([0, 0])  cap_slice() import(svg_circle, center=true);
translate([0, 5])  cap_slice() import(svg_circle, center=true, $fn=60);
translate([0, 10]) cap_slice() import(svg_circle, center=true, $fa=3, $fs=0.1);
translate([0, 15]) cap_slice() import(svg_circle, center=true, $fa=0.1, $fs=8);

// Column 2: <path> elliptical arc (id="id-circle", rotated -90 deg so outer arc apex is at y = +10)
// Note: default has 9 steps per 90-deg arc (10 deg/step -> 3-4 facets in cap)
module arc_r10() {
  scale([10 / 13.9588, 10 / 13.9588])
    rotate(-90)
    import(svg_sel, id="id-circle", center=true, $fn=$fn, $fa=$fa, $fs=$fs);
}
translate([8, 0])  cap_slice() arc_r10($fn=0,  $fa=12,  $fs=2);
translate([8, 5])  cap_slice() arc_r10($fn=60, $fa=12,  $fs=2);
translate([8, 10]) cap_slice() arc_r10($fn=0,  $fa=3,   $fs=0.1);
translate([8, 15]) cap_slice() arc_r10($fn=0,  $fa=0.1, $fs=1.3);

// Column 3: <path> cubic Bezier (outer rounded corner of simple.svg, rotated -45 deg so apex is at y = 5*sqrt(2)+5 = 12.071)
// Note: 1 cubic Bezier spans 90 deg (default 20 segs = 4.5 deg/seg -> 4 facets in cap y in [11.96, 12.071]).
module bezier_corner_slice() {
  scale([2.6, 34]) intersection() {
    translate([0, -11.96])
      rotate(-45)
      import(svg_simple, center=true, $fn=$fn, $fa=$fa, $fs=$fs);
    translate([0, 0.06]) square([3.0, 0.14], center=true);
  }
}
translate([16, 0])  bezier_corner_slice($fn=0,  $fa=12, $fs=2);
translate([16, 5])  bezier_corner_slice($fn=30, $fa=12, $fs=2);
translate([16, 10]) bezier_corner_slice($fn=60, $fa=12, $fs=2);
translate([16, 15]) bezier_corner_slice($fn=0,  $fa=1,  $fs=0.1);






