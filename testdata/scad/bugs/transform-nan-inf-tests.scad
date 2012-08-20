// Test translation by NaN and Infinity

// NaN test - cube(2) should not be rendered
cube(1);
angle = asin(1.1);
render()
rotate([0, 0, angle])
cube(2);

// FIXME: how do you test infinity?
