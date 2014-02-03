use <../../ttf/paratype-serif/PTF55F.ttf>

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
		text(t = t, font = "PT Serif:style=Regular", size = 20, direction = a[2]);
	}
}
