// dict_split_definition_instantiation.scad - usage of dict()

// dict() construct an associative array, where keys are strings. One of
// possible usages is to separate definition of the model from its
// instantiation. The model defintion then can be used elsewhere, for example
// to define mounting holes or attachment points, as shown in this example.

// The example could be split into three source files, as the modules are
// supposed to not know about each other. However, for simplicity, the files
// were fused into one and the content split by markers.


$fn = 32;

/* ---------------------- BOX MODULE DEFINITION ---------------------------- */

function box_definition(length, width, height)
    = dict(
        length = length,
        width = width,
        height = height,
    );

module box_instance(definition) {
    translate([-definition.length/2, -definition.width/2, 0])
        difference() {
            cube([definition.length, definition.width, definition.height]);
            translate([4, definition.width/2, -0.01])
                cylinder(r=2, h=definition.height+0.02);
            translate([definition.length-4, definition.width/2, -0.01])
                cylinder(r=2, h=definition.height+0.02);
        }
}

module at_box_mounting_holes(definition) {
    translate([-definition.length/2, -definition.width/2, 0]) {
        translate([4, definition.width/2, 0])
            children();
        translate([definition.length-4, definition.width/2, 0])
            children();
    }
}


/* --------------------- HOLDER MODULE DEFINITION -------------------------- */

function holder_definition(base_length, arm_length, arm_angle)
    = dict(
        base_length = base_length,
        arm_length = arm_length,
        arm_angle = arm_angle,
    );

module holder_instance(definition) {
    cube([10, 10, definition.base_length]);
    translate([5, 0, definition.base_length]) {
        rotate([-90, 0, 0])
            cylinder(d=10, h=20);
        rotate([0, definition.arm_angle, 0])
            translate([-5, 10, 0]) {
                cube([10, 10, definition.arm_length]);

                translate([5, 5, definition.arm_length+5])
                    cube([50, 50, 10], center=true);
            }
    }

}

module at_holder_payload(definition) {
    translate([5, 0, definition.base_length])
        rotate([0, definition.arm_angle, 0])
            translate([-5, 10, 0])
                translate([5, 5, definition.arm_length+10])
                    children();
}


/* ------------------------------ HELPERS ---------------------------------- */

module m3_hole(depth) {
    translate([0, 0, -depth])
        cylinder(d=3, h=depth+0.01);
}

/* -------------------------- ASSEMBLY MODULE ------------------------------ */

// define both models, but don't instantiate them yet
holder1 = holder_definition(100, 50, 40);
box1 = box_definition(30, 40, 20);

// instantiate holder and cut mounting holes for the box
difference() {
    color("green") holder_instance(holder1);
    at_holder_payload(holder1)
        at_box_mounting_holes(box1)
            m3_hole(depth=12);
}

// instantiate box attached to the holder
at_holder_payload(holder1)
    color("blue") box_instance(box1);
