// test cases for linear extrude with various (invalid) parameters
ox = 32;
oz = 26;
params = [ [0, undef], [1, 1/0], [2, -1/0], [3, 0/0], [4, ""], [5, true], [6, [1:3]], [7, 3] ];


for (a = params) translate([-2*ox, 0, oz * a[0]])
color("red") linear_extrude(height = 10, convexity = a[1]) square(20);

for (a = params) translate([-ox, 0, oz * a[0]])
color("yellow") linear_extrude(height = 10, convexity = undef, scale = 1, twist = a[1]) square(20);

for (a = params) translate([0, 0, oz * a[0]])
color("gray") linear_extrude(height = 10, convexity = undef, scale = 1, twist = 0, slices = a[1]) square(20);

for (a = params) translate([ox, 0, oz * a[0]])
color("purple") linear_extrude(height = 10, convexity = undef, scale = 1, twist = 30, slices = a[1]) square(20);

for (a = params) translate([2*ox, 0, oz * a[0]])
color("blue") linear_extrude(height = 10, convexity = 2, scale = a[1]) square(20);

for (a = params) translate([(a[0] - 3) * 30, -138, 0])
color("green") linear_extrude(height = a[1]) square(20);