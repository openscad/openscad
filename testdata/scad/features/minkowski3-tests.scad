module roundedBox3dSimple() {
    minkowski() {
        cube([10,10,5]);
        cylinder(r=5, h=5);
    }
}

module roundedBox3dCut() {
    minkowski() {
        difference() {
            cube([10,10,5]);
            cube([5,5,5]);
        }
        cylinder(r=5, h=5);
    }
}

module roundedBox3dHole() {
    minkowski() {
        difference() {
            cube([10,10,5], center=true);
            cube([8,8,10], center=true);
        }
        cylinder(r=2);
    }
}

translate([-20,30,0]) roundedBox3dHole();
translate([0,25,0]) roundedBox3dCut();
translate([25,25,0]) roundedBox3dSimple();

// One child
translate([0,0,0]) minkowski() { cube([10,10,5]); }

// Empty
minkowski();
// No children
minkowski() { }
