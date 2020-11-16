h = 5;
s = 10;

for (i = [0:3]) {
    object(-(s/2 + i), i * s * 1.2);
}
for (i = [4:7]) {
    translate([s * 1.2, 0, 0])
        object(-(s/2 + i), (i - 4) * s * 1.2);
}

module object(o, t) {
    translate([0, t, 0])
        offset_extrude(5, r=o, slices = 5)
        square(10, true);
}