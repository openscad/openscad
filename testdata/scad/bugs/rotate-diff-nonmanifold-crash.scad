difference() {
  rotate_extrude($fn=5) translate([4,0,0]) square([10, 10], center=true);
  translate([6,6,6]) sphere(r=10);
}
