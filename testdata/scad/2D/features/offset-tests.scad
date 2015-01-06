module m(x, y) {
    translate(60 * [x, y]) children();
}

module shape1(w = 20) {
    difference() {
        square([ w,  w], center = true);
        square([10, 10], center = true);
    }
}

module shape2() {
    polygon(points=[
            [-15, 80],[15, 80],[0,-15],[-8, 60],[8, 60],[0, 5]
    ], paths=[
            [0,1,2],[3,4,5]
    ]);
}

m(-1, 0) shape1();
m(-1, 2) shape2();

m(0, 0) offset() shape1();
m(0, 1) offset(5) shape1();
m(0, 2) offset(5) shape2();

m(1, 0) offset(r = 1) shape1(30);
m(1, 1) offset(r = 5) shape1(30);
m(1, 2) offset(r = 5) shape2();

m(2, 0) offset(r = -5) shape1(40);
m(2, 1) offset(r = -10.01) shape1(50);
m(2, 2) offset(r = -1) shape2();

m(3, 0) offset(delta = 4) shape1();
m(3, 1) offset(delta = 1) shape1();
m(3, 2) offset(delta = 5) shape2();

m(4, 0) offset(delta = -2) shape1(30);
m(4, 1) offset(delta = -5) shape1(40);
m(4, 2) offset(delta = -1) shape2();

m(5, 0) offset(delta = 4, chamfer = true) shape1();
m(5, 1) offset(delta = 1, chamfer = true) shape1();
m(5, 2) offset(delta = 5, chamfer = true) shape2();

m(6, 0) offset(delta = -2, chamfer = true) shape1(30);
m(6, 1) offset(delta = -5, chamfer = true) shape1(40);
m(6, 2) offset(delta = -1, chamfer = true) shape2();

// Bug with fragment calculateion with delta < 1 due to abs() instead of std::abs()
m(-2, 1) scale([30, 30]) offset(r = 0.8) square(1);

// Malformed offsets
offset();
offset() square(0);
