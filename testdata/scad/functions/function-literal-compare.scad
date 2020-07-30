// higher-order function returns a function (which returns captured value x)
function func_maker(x) = function() x;
a = func_maker(0);
b = a;
c = func_maker(0);
d = func_maker(1);

echo(a=a);
echo(b=b);
echo(c=c);
echo(d=d);
echo(a(), b(), c(), d());

// a and b are the same function object, so they are equal
echo("a==b", a==b);
echo("a!=b", a!=b);

// a and c are functionally the same, coming from the same piece of script,
// with same parameter. But they are not equal, since they are separate function objects,
// and might have captured different values in their closures, which are not checked against.
echo("a==c", a==c);
echo("a!=c", a!=c);

// a and d are definitely different
echo("a==d", a==d);
echo("a!=d", a!=d);

// ordering of functions is undefined
echo("a <b", a <b);
echo("a >b", a >b);
echo("a<=b", a<=b);
echo("a>=b", a>=b);
