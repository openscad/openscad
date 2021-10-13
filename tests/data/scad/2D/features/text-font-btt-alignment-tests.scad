use <../../../ttf/liberation-2.00.1/LiberationSans-Regular.ttf>

t = "OpenSCAD";
valign = [
	[0, "top"],
	[50, "center"],
	[100, "bottom"]
];

halign = [
	[0, "left"],
	[60, "center"],
	[120, "right"]
];

for (a = valign) {
	translate([a[0], 0, 0]) {
		color("red") square([20, 1]);
		color("blue") translate([0,-135]) square([1, 135]);
		text(text = t, font = "Liberation Sans:style=Regular", size = 20, valign = a[1], direction="btt");
	}
}

for (a = halign) {
	translate([200+a[0], 0, 0]) {
		color("red") square([20, 1]);
		color("blue") translate([0,-135]) square([1, 135]);
		text(text = t, font = "Liberation Sans:style=Regular", size = 20, halign = a[1], direction="btt");
	}
}

