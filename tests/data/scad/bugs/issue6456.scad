render() {
  difference() {
    cube(10);
  }
}
render() {
  translate([0,20,0]) intersection() {
    cube(10);
  }
}
translate([20,0,0]) {
  render() {
    difference() {
      group();
      cube(10);
    }
  }
  render() {
    translate([0,20,0]) intersection() {
      group();
      cube(10);
    }
  }
}
