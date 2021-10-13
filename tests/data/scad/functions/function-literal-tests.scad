v = [1, 2, 3, 4, 5, 6, 7, 8, 9];
x = 1;

// warnings
echo(undef());
echo(true());
echo(10.5());
echo("string"());
echo([]());
echo([1:4]());

// check is_function()
echo(is_function1 = is_function(v));
echo(is_function2 = is_function(filter));

// passing function to module, testing lexical + dynamic scope
// both named and anonymous function is passed though multiple
// module scope levels which have variables shadowing the outer
// scope. for lexical scoped lookup that must not affect the
// result value, for dynamic scope, the local value must be
// picked up.
module scope_test1(s, f) {
    a = 0.5; $a = 50;
    scope_test_func = function(x) -50;
    echo(s, scope_test1 = f(5));

    scope_test2(s, f);
    scope_test3(s, f);

    module scope_test3(s, f) {
        a = 0.6; $a = 60;
        scope_test_func = function(x, i) -60;
        echo(s, scope_test3 = f(6));
    }
}

module scope_test2(s, f) {
    a = 0.7; $a = 70;
    scope_test_func = function(x, i) -70;
    echo(s, scope_test2 = f(7));
}

a = 0.3; $a = 30;
scope_test1("A", function(x) 100 + x + a + $a);

scope_test_func = function(x, i = 0) x > 0 ? scope_test_func(x - 1, i + 1) : i;
scope_test1("B", scope_test_func);

module special_var_test(f) {
    $special_var_func = function(x, i = 0) x > 0 ? $special_var_func(x - 1, i + 0.1) : i;
    echo(specialvar2 = f(5));
}

$special_var_func = function(x, i = 0) x > 0 ? $special_var_func(x - 1, i + 1) : i;
echo(specialvar1 = $special_var_func(5));
special_var_test($special_var_func);

// pass function to another function
map = function(v, f) [ for (a = v) f(a) ];
echo(map = map(v, function(x) x * x));

filter = function(v, pred) [ for (a = v) if (pred(a)) a ];
echo(filter = filter(v, function(x) x % 2 == 0));

// immediate invoke
echo(immediate = (function(x) sqrt(x))(49));

// function returning a function
func  = function(x) x == 0
    ? function(x, y) x + y
    : function(x, y) x - y;

echo(func0 = func(0)(8, 3));
echo(func1 = func(1)(8, 3));

// recursive function, tail recursion
fold = function(i, v, f, off = 0) len(v) > off ? fold(f(i, v[off]), v, f, off + 1) : i;
echo(fold1 = fold(0, v, function(x, y) x + y));
echo(fold2 = fold(0, rands(0, 1, 100000), function(x, y) x + 1));

// chained tail recursion
chaining1 = function(x) x <= 0 ? 0 : chaining2(x - 1);
chaining2 = function(x) x <= 0 ? 0 : chaining1(x - 1);
echo(chaining = chaining1(500000));

// function call scoping
function scope_capture() = let(scope_capture = function() 5) scope_capture();
echo(scope_capture = scope_capture());
