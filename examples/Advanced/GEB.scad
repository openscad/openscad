font = "Bank Gothic";

module G() offset(0.3) text("G", size=10, halign="center", valign="center", font = font);
module E() offset(0.3) text("E", size=10, halign="center", valign="center", font = font);
module B() offset(0.5) text("B", size=10, halign="center", valign="center", font = font);

$fn=64;

module GEB() {
intersection() {
    linear_extrude(height = 20, convexity = 3, center=true) B();
    
    rotate([90, 0, 0])
      linear_extrude(height = 20, convexity = 3, center=true) E();
    
    rotate([90, 0, 90])
      linear_extrude(height = 20, convexity = 3, center=true) G();
  }
}

color("Ivory") GEB();

color("MediumOrchid") translate([0,0,-20]) render() difference() {
    square(40, center=true);
    projection() GEB();
}
color("DarkMagenta") rotate([90,0,0]) translate([0,0,-20]) render() difference() {
    square(40, center=true);
    projection() rotate([-90,0,0]) GEB();
}
color("MediumSlateBlue") rotate([90,0,90]) translate([0,0,-20]) render() difference() {
    square(40, center=true);
    projection() rotate([0,-90,-90]) GEB();
}
    