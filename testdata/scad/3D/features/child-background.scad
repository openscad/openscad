module transparent() {
 %child();
}

difference() {
  sphere(r=10);
  transparent() cylinder(h=30, r=6, center=true);
}
