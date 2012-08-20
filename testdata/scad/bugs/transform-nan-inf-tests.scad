// Test translation by NaN and Infinity

// NaN test - cube() should not be rendered
sphere();
angle = asin(1.1);
render()
rotate([0, 0, angle])
cube();

// FIXME: how do you test infinity?
