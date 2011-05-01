linear_extrude(height=10) square([10,10]);
translate([19,5,0]) linear_extrude(height=10) circle(5);
translate([31.5,2.5,0]) linear_extrude(height=10) polygon(points = [[-5,-2.5], [5,-2.5], [0,2.5]]);

translate([0,-12,0]) linear_extrude(height=20, twist=45) square([10,10]);
translate([19,-7,0]) linear_extrude(height=20, twist=90) circle(5);
translate([31.5,-9.5,0]) linear_extrude(height=20, twist=180) polygon(points = [[-5,-2.5], [5,-2.5], [0,2.5]]);
