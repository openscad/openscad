use <../../ttf/liberation-2.00.1/LiberationSans-Regular.ttf>

t = "OpenSCAD";
valign = [
	[10, "top"],
	[40, "center"],
	[70, "baseline"],
	[100, "bottom"]
];

halign = [
	[10, "left"],
	[40, "center"],
	[70, "right"]
];

for (a = valign) {
	translate([10, a[0], 0]) {
		color("red") square([135, 0.5]);
		color("blue") square([0.5, 20]);
		text(text = t, font = "Liberation Sans:style=Regular", size = 20, valign = a[1]);
	}
}

for (a = halign) {
	translate([160 + 2.23 * a[0], a[0], 0]) {
		color("red") square([135, 0.5]);
		color("blue") square([0.5, 20]);
		text(text = t, font = "Liberation Sans:style=Regular", size = 20, halign = a[1]);
	}
}

