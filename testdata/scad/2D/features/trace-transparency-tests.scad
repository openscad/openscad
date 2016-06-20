i="../../../image/trace-test-card-transparency.png";

trace(i);
translate([300, 0])
    trace(i, threshold = 0.9);
translate([0, 300])
    trace(i, threshold = 0.5);
translate([300, 300])
    trace(i, threshold = 0.1);
