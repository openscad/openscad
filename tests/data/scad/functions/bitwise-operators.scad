// Hex constants - digits, lower case, upper case.
assert(0x1ff == 511);
assert(0xFFFFFFFF == 2^32-1);

// Basic operations
assert((12 & 5) == 4);
assert((12 | 5) == 13);
assert((2 << 3) == 16);
assert((16 >> 2) == 4);
assert(~0 == -1);
assert(~-1 == 0);
assert(~5 == -6);

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
assert((-1 | 0) == -1);
assert((-1 | 0) == ~0);
assert((-2^31) | 0 == -2^31);


// Fractions (trunc)
assert((1.4 | 0) == 1);
assert((1.6 | 0) == 1);
assert((-1.4 & 3) == 3);
assert((-1.6 & 3) == 3);


// Limits
assert((1 << 32 << 32) == 0);
assert((1>>1) == 0);

// Hex constants
assert(0x0 == 0);
assert(0x00 == 0);
assert(0xff == 255);    // lower case
assert(0xFF == 255);    // upper case

// Undefined operations
// Note that these generate warnings.
echo("Expect warnings:");
assert(is_undef(1 << 64));
assert(is_undef(1 << -1));
assert(is_undef(1 >> 64));
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

// Expect too-large warning.
if (true) { a = 0x10000000000000000; }
// Expect loss-of-precision warnings.
if (true) { a = 0x1000000000000001; }
if (true) { a = 1152921504606846977; }
// Expect loss-of-precision warnings, but they should convert to the same imprecise number.
// arm64 somehow manages to convert 2^64-1 to double and back without losing anything, so
// this uses a value that's less close to the seemingly-magic all-ones value.
assert(0xfffffffffffffff0 == 18446744073709551600);
