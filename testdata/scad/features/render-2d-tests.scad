render() {
  difference() {
    square(10, center=true);
    circle(r=3);
  }
}

translate([12,0,0]) render() {
  square(10, center=true);
  circle(r=3);
}
