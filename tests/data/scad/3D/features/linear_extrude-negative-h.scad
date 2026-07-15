$fa = 5; $fs = 0.5;

xo = 13;
yo = 25;

module s() union() { square(5, center=true); translate([4, 0]) circle(1); }

module lp(x, h, v=undef, twist=0, scale=1) {
    translate([xo * x, 0, 0])
        color("green")
            linear_extrude(h, v=v, twist=twist, scale=scale)
                children();
    translate([xo * x, -yo, 0])
        color("green")
            linear_extrude(h, v=v, twist=twist, scale=scale, center=true)
                children();
}
module ln(x, h, v=undef, twist=0, scale=1) {
    translate([xo * x, 0, 0])
        color("red")
            linear_extrude(-h, v=v, twist=twist, scale=scale)
                children();
    translate([xo * x, yo, 0])
        color("red")
            linear_extrude(-h, v=v, twist=twist, scale=scale, center = true)
                children();
}

lp(0, 10) s();
ln(0, 10) s();

lp(1, 10, twist = 90) s();
ln(1, 10, twist = 90) s();

lp(2, 10, twist = -90) s();
ln(2, 10, twist = -90) s();

lp(3, 10, scale = 0.5) s();
ln(3, 10, scale = 0.5) s();

lp(4, 10, scale = [0.5, 2]) s();
ln(4, 10, scale = [0.5, 2]) s();

lp(5, 10, v = [1, 2, 3]) s();
ln(5, 10, v = [1, 2, 3]) s();
