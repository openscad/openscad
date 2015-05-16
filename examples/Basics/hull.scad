$fn=20;
box_height=15;
box_capdiameter = 20;
box_length = 20;
wall_thickness = 5;

difference() {
    hull() {
        cylinder(d=box_capdiameter, h=box_height);
        translate([20, 0, 0]) cylinder(d=box_capdiameter, h=box_height);
    }
    translate([0, 0, 3])
    hull() {
        cylinder(d=box_capdiameter-wall_thickness, h=box_height);
        translate([20, 0, 0]) cylinder(d=box_capdiameter-wall_thickness, h=box_height);
    }
}