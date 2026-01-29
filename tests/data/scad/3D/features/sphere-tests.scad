sphere();
translate([2,0,0]) sphere(r=0);
translate([0,11,0]) sphere(5);
translate([0,-11,0]) sphere(r=5);
translate([11,-11,0]) sphere(5, $fn=5);
translate([11,0,0]) sphere(5, $fn=10);
translate([11,11,0]) sphere(5, $fn=15);
translate([22,-11, 0]) sphere(5, $fa=20, $fs=0.3);
translate([22,  0, 0]) sphere(5, $fa=30, $fs=0.3);
translate([22, 11, 0]) sphere(5, $fa=40, $fs=0.3);
translate([11, 22, 0]) sphere(5, $fn=0.1);
translate([33,  0, 0]) sphere(d=10);
translate([33, 11, 0]) sphere(r=1, d=10);
translate([33, -11, 0]) sphere($fe=0.05, d=10, $fa=0.5, $fs=0.5); // Will be shiny smooth if $fe doesn't override $fa/$fs
translate([22, 22, 0]) sphere($fe=0.001, d=10, $fn=6); // Will be shiny smooth if $fn doesn't override $fe
