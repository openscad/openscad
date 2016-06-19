a = 3;
b = 6;

t0 = echo();
echo(t0 = t0);

t1 = echo("t1");
echo(t1 = t1);

t2 = echo(a*b);
echo(t2 = t2);

t3 = echo(c = a*b);
echo(t3 = t3);

t4 = echo() a*b;
echo(t4 = t4);

t5 = echo(c = 2) a*b*c;
echo(t5 = t5);

t6 = echo(c = 2, d = c + 9) a*b*c*d;
echo(t6 = t6);
