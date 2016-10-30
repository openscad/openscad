a = 3;
b = 6;
c = 8;
v = [2, 5, 7];

t0 = echo();
echo(t0 = t0);

t1 = echo() undef;
echo(t1 = t1);

t2 = echo("t2");
echo(t2 = t2);

t3 = echo(a*b);
echo(t3 = t3);

t4 = echo(c = a*b);
echo(t4 = t4);

t5 = echo() a*b;
echo(t5 = t5);

t6 = echo(c = 2) a*b*c;
echo(t6 = t6);

t7 = echo() [a,b];
echo(t7 = t7);

t8 = echo() [for (i=[1:a]) [i,b]];
echo(t8 = t8);

function f1(x) = [ for (a = x) echo(a = a) a ];
echo("f1(v) = ", f1(v));

function f2(x, i = 0) = echo(i) len(x) > i ? x[i] + f2(x, i + 1) : 0;
echo("f2(v) = ", f2(v));

function f3(x, i = 0) = len(x) > i ? let(a = x[i] + f3(x, i + 1)) echo(a) a : 0;
echo("f3(v) = ", f3(v));
