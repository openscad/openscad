// Example for offset() usage
// (c) 2014 Torsten Paul
// CC-BY-SA 4.0

$fn = 40;

foot_height = 20;

echo(version=version());

module outline(wall = 1) {
	difference() {
		offset(wall / 2) children();
		offset(-wall / 2) children();
	}
}

// offsetting with a positive value and join_type = "round"
// allows to create rounded corners easily
linear_extrude(height = foot_height, scale = 0.5) {
  offset(10, join_type = "round") {
    square(50, center = true);
  }
}

translate([0, 0, foot_height]) {
	linear_extrude(height = 20) {
		outline(wall = 2) circle(15);
	}
}

%cylinder(r = 14, h = 100);
%translate([0, 0, 100]) sphere(r = 30);
