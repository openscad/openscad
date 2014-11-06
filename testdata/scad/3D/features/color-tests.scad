// Empty
color();
// No children
color() { }

module object() cube([10,10,10]);

translate([12,12,0]) object();
color([1,0,0]) translate([24,12,0]) object();
translate([0,12,0]) color("Purple") object();
color([0,0,1,0.5]) object();
translate([12,0,0]) color([0,0,1],0.5) object();
translate([24,0,0]) color(c="Green",alpha=0.2) object();
translate([-12,12,0]) color() object();
translate([-12,0,0]) color(alpha=0.5) object();
translate([24,-12,0]) color([1,0,0]) color([0,0,1]) object();
