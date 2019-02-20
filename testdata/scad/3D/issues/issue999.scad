linear_extrude(height=1)
difference() {
    square([1,1], center=true);
    translate([0, -0.5]) circle(r=0.25, $fn=60); // polygon cleanup doesn't touch our circle at this scale
    translate([-0.5, 0]) circle(r=0.25, $fn=600);// shows up much more coarse if polygon cleaning distance is too large
}
