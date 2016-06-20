i="../../../image/trace-test-card.png";

trace(i);
translate([300, 0])
    trace(i, threshold = 0.9);
translate([0, 300])
    trace(i, threshold = 0.5, $fn = 1);
translate([300, 300])
    trace(i, threshold = 0.1);
