difference() {
  sphere(r=10);
  %#cylinder(h=30, r=6, center=true);
  %#if (true) cube([6,25,3], center=true);
}
translate([13,0,0]) difference() {
  sphere(r=10);
  #%cylinder(h=30, r=6, center=true);
  #%if (true) cube([6,25,3], center=true);
}
