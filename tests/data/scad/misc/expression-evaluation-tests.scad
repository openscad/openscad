values = [
    undef,                      // special undefined value
    1/0,                        // infinity
    -1/0,                       // -infinity
    0/0,                        // not a number
    0, -4.2, -2, 3, 42.42, 242, // number
    true, false,                // boolean
    "", "text",                 // string
    [], [ 0 ], [ 1 ],           // vector
    [ 0 : 0 ], [ 1 : 2 ]        // range
];

array = [ "a", "b", "c", "d" ];

for (v = values) {
    echo(v = v, op = "not v", result = !v);
    echo(v = v, op = "-v", result = -v);
    echo(v = v, op = "v *", result = v * 3);
    echo(v = v, op = "* v", result = 2 * v);
    echo(v = v, op = "v /", result = v / 3);
    echo(v = v, op = "/ v", result = 2 / v);
    echo(v = v, op = "v %", result = v % 3);
    echo(v = v, op = "% v", result = 2 % v);
    echo(v = v, op = "v +", result = v + 3);
    echo(v = v, op = "+ v", result = 2 + v);
    echo(v = v, op = "v -", result = v - 3);
    echo(v = v, op = "- v", result = 2 - v);
    echo(v = v, op = "v and true", result = v && true);
    echo(v = v, op = "v and false", result = v && false);
    echo(v = v, op = "v or true", result = v || true);
    echo(v = v, op = "v or false", result = v || false);
//    echo(v = v, op = "<", result = v < 3);
//    echo(v = v, op = "<=", result = v <= 3);
//    echo(v = v, op = "==", result = v == 3);
//    echo(v = v, op = "!=", result = v != 3);
//    echo(v = v, op = ">=", result = v >= 3);
//    echo(v = v, op = ">", result = v > 3);
    echo(v = v, op = "[v]", result = array[v]);
    echo(v = v, op = "v[0]", result = v[0]);
    echo(v = v, op = "v[4]", result = v[4]);
}
