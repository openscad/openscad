function f(x) = x;
s = "string";
smiley = 9786;

// empty, inf, -inf, nan
echo(chr(), chr(1/0), chr(-1/0), chr(0/0));

// invalid values
echo(chr(0), chr(-0), chr(0.5), chr(-0.9), chr(-100), chr(2097152), chr(1e10), chr(-2e10));

// valid values
echo(chr(f(33)), chr(49), chr(65), chr(97), chr(smiley), chr(128512));

// multiple values
echo(chr(90, 89, 88), chr(65, "test", false, -5, 0, 66, [67:69], [70, 71, 72, 73], smiley));

// ranges
echo(chr([48:57]), chr([70 : 2 : 80]));

// vectors
echo(chr([65, 66, 67, 97, 98, 99]), chr([49, "test", true, -1, 50]));

// other (not supported) types
echo(chr(undef), chr(true), chr("test"), chr(s));
