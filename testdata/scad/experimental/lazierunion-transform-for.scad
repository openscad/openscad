/*!
  Highlight an issue to keep in mind:
  If we first render (F6) the second line only:
    for (i=[0:2]) translate([i*10,0,0]) cube(8);
  and then the whole design: 
    translate([5,0,0]) for (i=[0:2]) translate([i*10,0,0]) cube(8);

  ..the first for node was cached, but since the CGAL renderer cannot render GeometryList nodes, it failed.

  This was fixed by not caching ListNode instances.
*/

translate([5,0,0])
for (i=[0:2]) translate([i*10,0,0]) cube(8);
