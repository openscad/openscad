use <../../../ttf/amiri-0.106/amiri-regular.ttf>
use <../../../ttf/liberation-2.00.1/LiberationSans-Regular.ttf>

t1="OpenSCAD";
t2="الخط الأميري";
t3="типографика";

rotate([45, 0, -45]) {
	translate([0, 80, 0])
		text(text = t1, font = "Liberation Sans:style=Regular", size = 20);

	translate([0, 40, 0])
		text(text = t2, font = "Amiri:style=Regular", size = 20, direction = "rtl", language = "ar", script = "arabic");

	text(text = t2, font = "Amiri:style=Regular", direction = "rtl", size = 20, language = "ar", script = "arabic");

	translate([0, -40, 0])
		text(text = t3, font="Liberation Sans:style=Regular", size=20, language="ru");

	translate([0, -80, 0])
		text("positional", 30, "Liberation Sans:style=Regular");

	translate([0, -100, 0])
		text("parameters", 12, "Amiri:style=Regular");
}
