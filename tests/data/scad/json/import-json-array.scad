// Test import of JSON files whose top-level value is an array,
// and of arrays nested in objects and other arrays.

a = import("../../json/array.json");
echo(a);

// The result is an ordinary OpenSCAD vector.
echo(is_list(a), len(a));
echo(a[2]);
// JSON null becomes undef.
echo(a[5]);
// Empty array and empty object.
echo(a[6], len(a[6]));
echo(a[7]);
// Nested arrays.
echo(a[8][1][1][0]);
// Object inside an array.
echo(a[9].name);
// Array of objects, iterated with a list comprehension.
echo([for (o = a[10]) o.id]);

// Arrays of numbers inside an object can be used as vectors.
p = import("../../json/array-points.json");
echo(p.points);
echo(len(p.points));
echo(p.points[1] + p.points[2]);
echo(p.nulls);
// Number formats inside arrays.
echo(p.numbers);
// Strings with escapes and non-ASCII characters inside arrays.
echo(p.strings);
echo([for (s = p.strings) len(s)]);

// An empty top-level array.
echo(import("../../json/array-empty.json"));

// An unterminated array fails to parse.
echo(import("../../json/array-unterminated.json"));

// Trailing data after a top-level array fails to parse.
echo(import("../../json/array-trailing-data.json"));

// A trailing comma is not valid JSON.
echo(import("../../json/array-trailing-comma.json"));
