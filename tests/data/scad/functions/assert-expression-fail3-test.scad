function f(x) = sin(x);

module m(angle) {
	v = assert(f(angle) > 0) 10;
}

m(270);
