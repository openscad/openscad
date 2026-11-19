// color(undef) must leave colors alone, like in preview (#5853).
color("green") cube(10);
color(undef) translate([15, 0, 0]) cube(10);                // default color
color(undef) translate([30, 0, 0]) color("red") cube(10);   // stays red
