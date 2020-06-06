a = 3;
b = 6;

t0 = warning(a);
echo(t0=t0);

t1=warning("t1");
echo(t1=t1);

t2=warning(a*b);
echo(t2=t2);

t3 = a<0?"True":warning("False");
echo(t3 = t3);

t4 = warning() a*b;
echo(t4 = t4);

t5 = warning() [a,b];
echo(t5 = t5);

t6 = warning() [for (i=[1:a]) [i,b]];
echo(t6 = t6);

c = a + 9;
t7= c + 5 >15?str("value: ", c+ 5):a*b*c;
echo(t7 = t7);

