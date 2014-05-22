use <../../ttf/amiri-0.106/amiri-regular.ttf>
use <../../ttf/amiri-0.106/amiri-bold.ttf>
use <../../ttf/paratype-serif/PTF55F.ttf>

t1="OpenSCAD";
t2="الخط الأميري";
t3="типографика";

rotate([45, 0, -45]) {
	translate([0, 80, 0])
		text(t = t1, font = "PT Serif:style=Regular", size = 20);

	translate([0, 40, 0])
		text(t = t2, font = "Amiri:style=Bold", size = 20, direction = "rtl", language = "ar", script = "arabic");

	text(t = t2, font = "Amiri:style=Regular", direction = "rtl", size = 20, language = "ar", script = "arabic");

	translate([0, -40, 0])
		text(t = t3, font="PT Serif:style=Regular", size=20, language="ru");

	translate([0, -80, 0])
		text("positional", 30, "PT Serif:style=Regular");

	translate([0, -100, 0])
		text("parameters", 12, "Amiri:style=Bold");
}