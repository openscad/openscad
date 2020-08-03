color("Blue") {
        translate([-30,-10,-10]) cube([70,50,2]);
        translate([40,-10,-10]) cube([2,52,30]);
        translate([-30,40,-10]) cube([70,2,30]);
}

translate([-20,0,0]) difference() {
    intersection() {
        sphere(10);
        cube(15, center=true);
    }
    cylinder(h=20, r=5, center=true);
}

%translate([-20,20,0]) difference() {
    intersection() {
        sphere(10);
        cube(15, center=true);
    }
    cylinder(h=20, r=5, center=true);
}

difference() {
    intersection() {
        sphere(10);
        cube(15, center=true);
    }
  %cylinder(h=20, r=5, center=true);
}

translate([20,0,0]) difference() {
    %intersection() {
        sphere(10);
        cube(15, center=true);
    }
    cylinder(h=20, r=5, center=true);
}

translate([0,20,0]) difference() {
    intersection() {
        %sphere(10);
        cube(15, center=true);
    }
    cylinder(h=20, r=5, center=true);
}

translate([20,20,0]) difference() {
    intersection() {
        sphere(10);
        %cube(15, center=true);
    }
    cylinder(h=20, r=5, center=true);
}
