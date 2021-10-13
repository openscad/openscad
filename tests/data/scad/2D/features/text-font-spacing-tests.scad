use <../../../ttf/liberation-2.00.1/LiberationSans-Regular.ttf>

t = "abcXYZ";

translate([0,-25,0])
text(text = t, font = "Liberation Sans:style=Regular", size = 20, spacing  = 0.5);
translate([0,0,0])
text(text = t, font = "Liberation Sans:style=Regular", size = 20);
translate([0,25,0])
text(text = t, font = "Liberation Sans:style=Regular", size = 20, spacing  = 1);
translate([0,50,0])
text(text = t, font = "Liberation Sans:style=Regular", size = 20, spacing  = 2);
