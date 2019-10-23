function substring(text, start, end = -1, idx = -1, res = "") =
    idx < end && idx < len(text)
        ? substring(text, start, end, idx < 0 ? start + 1 : idx + 1, str(res, text[idx < 0 ? start : idx]))
        : res;

// normal recursion, no tail-recursion elimination possible
function f3a(a, ret = 0) = a <= 0 ? 0 : a + f3a(a - 1);
echo("without tail-recursion eliminiation: ", f3a(100));

// this allows tail-recursion eliminiation
function f3b(a, ret = 0) = a <= 0 ? ret : f3b(a - 1, ret + a);
echo("with tail-recursion eliminiation: ", f3b(100));

// check tail-recursion eliminiation by using a high loop count
function f3c(a, ret = 0) = a <= 0 ? ret : f3c(a - 1, ret + a);
echo("with tail-recursion eliminiation: ", f3c(2000));

// use nested function call
function f1(x, y = []) = x <= 0 ? y : f1(x - 1, concat(y, [[x, x]]));
echo(f1(2000)[20]);

// recursion in the "false" part of the ternary operator
function c(a, b) = chr(a % 26 + b);
function f2a(x, y = 0, t = "") = x <= 0 ? t : f2a(x - 1, y + 2, str(t, c(y, 65)));
s1 = f2a(50000);
echo(len(s1), substring(s1, 0, 40));

// recursion in the "true" part of the ternary operator
function f2b(x, y = 0, t = "") = x > 0 ? f2b(x - 1, y + 1, str(t, chr((y % 26) + 97))) : t;
s2 = f2b(50000);
echo(len(s2), substring(s2, 0, 40));

// tail recursion with a complicated mix of let/assert/echo and nested ternary operators
function ftail_mixed(n) =
    let(x = 42 + n)
    assert(x >= 42)
    n == 0 ? (
        x
    ) : n < 10 ? (
        n < 5 ? (
            let(y = 33 + x)
            echo(n=n, x=x, y=y)
            ftail_mixed(n - 1)
        ) : (
            assert(n >= 0)
            ftail_mixed(n - 1)
        )
    ) : (
        let(y = let(z = n - 1) z)
        ftail_mixed(y)
    );
echo(ftail_mixed=ftail_mixed(10000)); // 42
