use <../../../ttf/liberation-2.00.1/LiberationSans-Regular.ttf>

t = "OpenSCAD";
dir = [
	[90, 10, "ltl"],
	[90, 60, "rtl"],
	[10, 160, "ttb"],
	[60, 140, "btt"]
];

for (a = dir) {
	translate([a[0], a[1], 0]) {
		color("red") square([135, 0.5]);
		color("blue") square([0.5, 20]);
		text(text = t, font = "Liberation Sans:style=Regular", size = 20, direction = a[2]);
	}
}
