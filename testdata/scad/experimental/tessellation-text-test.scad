module letter(c) {
    text(c, size = 50, font = "Liberation Sans", halign = "center", valign = "center");
}

linear_extrude(height = 10) {
    letter("C", $fn=8);
    translate([50,0]) letter("C", $fn=16);
    translate([0,50]) letter("C", $fn=24);
    translate([50,50]) letter("C", $fn=32);
}
