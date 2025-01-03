module CheckBox(symbol, string) {
  difference() {
    square(22);
    translate([1,1]) square(20);
  }
  translate([2,2]) {
    text(symbol, font="Liberation Sans:charset=76,78", size=20);
    translate([25,0]) text(string, size=20);
  }
}

CheckBox("v", "Checked");
translate([0,25]) CheckBox("x", "Crossed");

// Will not render, but issue a warning due to bad charset format
text("Won't display", font=":charset=xxx");
