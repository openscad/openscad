a = 3;
b = 6;

t0 = error(a);
echo(t0=t0);

t1=error("t1");
echo(t1=t1);

t2=error(a*b);
echo(t2=t2);

t3 = a<0?"True":error("False");
echo(t3 = t3);

t4 = error() a*b;
echo(t4 = t4);

t5 = error() [a,b];
echo(t5 = t5);

t6 = error() [for (i=[1:a]) [i,b]];
echo(t6 = t6);

c = a + 9;
t7= c + 5 >15?str("value: ", c + 5):a*b*c;
echo(t7 = t7);

