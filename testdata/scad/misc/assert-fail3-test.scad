function f(x) = sin(x);

module m(angle) {
	assert(f(angle) > 0) cube(10);
}

m(270);
