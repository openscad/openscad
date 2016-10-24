a = 3;
b = 6;

t0 = assert(true);
echo(t0 = t0);

t1 = assert("t1");
echo(t1 = t1);

t2 = assert(a*b);
echo(t2 = t2);

t3 = assert(condition = a*b);
echo(t3 = t3);

t4 = assert(true) a*b;
echo(t4 = t4);

c = 2;
t5 = assert(condition = 2) a*b*c;
echo(t5 = t5);

d = c + 9;
t6 = assert(condition = d + 5 > 15, message = str("value: ", d + 5)) a*b*c*d;
echo(t6 = t6);
