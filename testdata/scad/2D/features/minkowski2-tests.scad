module roundedBox2dSimple() {
    minkowski() {
        square([10,10]);
        circle(r=5);
    }
}

module roundedBox2dCut() {
    minkowski() {
        difference() {
            square([10,10]);
            square([5,5]);
        }
        circle(r=5);
    }
}

module roundedBox2dHole() {
    minkowski() {
        difference() {
            square([10,10], center=true);
            square([8,8], center=true);
        }
        circle(r=2);
    }
}

translate([-20,5,0]) roundedBox2dHole();
translate([0,0,0]) roundedBox2dCut();
translate([25,0,0]) roundedBox2dSimple();

// One child
translate([0,-20,0]) minkowski() { square(10); }

// >2 children
translate([-20,-20,0]) minkowski() {
    square(10);
    square(2, center=true);
    circle(1);
}

module invert() render() difference() { square(1e6,center=true); child(); }
module erode(d=.3) invert() minkowski() { circle(d); invert() child(); }

// This particular combination created a hairline crack inside the resulting polygon
translate([-5,-45]) scale(4) erode() minkowski() {
	circle(r=.4);
	circle(r=4);
}

// This is an even harder example
translate([30,-30]) scale(4) erode() minkowski() {
	difference() {
		circle(r=.4);
		circle(r=.399);
	}
	circle(r=4);
}

// Minkowski with an empty polygon should yield an empty result
translate([30,-45]) minkowski() {
	circle(r=1);
	circle(r=0);
}

// Test empty as first polygon
translate([30,30]) minkowski() {
	circle(r=0);
	circle(r=5);
}

// Test empty as first and only polygon
translate([-30,30]) minkowski() {
	circle(r=0);
}
