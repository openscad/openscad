// Hex constants - digits, lower case, upper case.
assert(0x1ff == 511);
assert(0xFFFFFFFF == 2^32-1);

// Basic operations
assert((12 & 5) == 4);
assert((12 | 5) == 13);
assert((2 << 3) == 16);
assert((16 >> 2) == 4);
assert(~5 == 2^32-6);

// The != tests below confirm that a precedence or
// associativity error would be detected.

// Precedence

// Add is higher than shift.
assert((1 << 3 + 1) == (1 << (3 + 1)));
assert((1 << 3 + 1) != ((1 << 3) + 1));
assert((1 + 1 << 3) == ((1 + 1) << 3));
assert((1 + 1 << 3) != (1 + (1 << 3)));

// Shift is higher than binary-and
assert((3 << 1 & 14) == ((3 << 1) & 14));
assert((3 << 1 & 14) != (3 << (1 & 14)));
assert((14 & 3 << 1) == (14 & (3 << 1)));
assert((14 & 3 << 1) != ((14 & 3) << 1));

// Binary-and is higher than binary-or.
assert((1 & 2 | 3) == ((1 & 2) | 3));
assert((1 & 2 | 3) != (1 & (2 | 3)));
assert((3 | 1 & 2) == (3 | (1 & 2)));
assert((3 | 1 & 2) != ((3 | 1) & 2));

// Binary-or is higher than greater/less.
// Note that the two != tests yield undefined
// operation warnings for bool|num and num|bool.
echo("Expect two warnings:");
assert((1 | 2 > 3) == ((1 | 2) > 3));
assert((1 | 2 > 3) != (1 | (2 > 3)));
assert((3 > 2 | 1) == (3 > (1 | 2)));
assert((3 > 2 | 1) != ((3 > 1) | 2));


// Associativity
// Binary-or and binary-and are associative.
assert((1 | 2 | 3) == ((1 | 2) | 3));
assert((1 | 2 | 3) == (1 | (2 | 3)));
assert((15 & 6 & 3) == ((15 & 6) & 3));
assert((15 & 6 & 3) == (15 & (6 & 3)));

// Shift associates left-to-right.
assert((1 << 2 << 3) == ((1 << 2) << 3));
assert((1 << 2 << 3) != (1 << (2 << 3)));
assert((65536 >> 2 >> 3) == ((65536 >> 2) >> 3));
assert((65536 >> 2 >> 3) != (65536 >> (2 >> 3)));
assert((65536 >> 4 << 1) == ((65536 >> 4) << 1));
assert((65536 >> 4 << 1) != (65536 >> (4 << 1)));
assert((65536 << 4 >> 1) == ((65536 << 4) >> 1));
assert((65536 << 4 >> 1) != (65536 << (4 >> 1)));


// Negative numbers
assert((-1 | 0) == 2^32-1);
assert((-1 | 0) == ~0);
assert((-2^31) | 0 == 2^31);


// Fractions (floor)
assert((1.4 | 0) == 1);
assert((1.6 | 0) == 1);
assert((-1.4 & 3) == 2);
assert((-1.6 & 3) == 2);


// Limits
assert((1 << 16 << 16) == 0);
assert((1>>1) == 0);


// Undefined operations
// Note that these generate warnings.
echo("Expect warnings:");
// Out-of-range values trigger a warning and are treated as undef.
// Perhaps they should be ANDed into range.
assert(is_undef(2^32 | 0));
assert(is_undef((-2^31 - 1) | 0));
assert(is_undef(1 << 32));
assert(is_undef(1 << -1));
assert(is_undef(1 >> 32));
assert(is_undef(1 >> -1));
assert(is_undef(1 | "hello"));
assert(is_undef("hello" | 1));
assert(is_undef(1 & "hello"));
assert(is_undef("hello" & 1));
assert(is_undef(1 << "hello"));
assert(is_undef("hello" << 1));
assert(is_undef(1 >> "hello"));
assert(is_undef("hello" >> 1));
assert(is_undef(~"hello"));
