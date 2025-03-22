use <../../../ttf/amiri-0.106/amiri-regular.ttf>

module t(y,s,f,t) {
    translate([160, y]) text(str(f, ":"), s, font = "Amiri:style=Regular", halign = "right");
    translate([200, y]) text(t, s, font = str("Amiri:style=Regular:fontfeatures=", f));
}

t(100, 50, "+liga", "fi fl fj ff");
t(20, 50, "-liga", "fi fl fj ff");
