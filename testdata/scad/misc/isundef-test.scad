echo(isundef(a)); //no warning
b="hallo";
echo(isundef(b));
c=undef;
echo(isundef(c));

echo(a); //warns
echo(b);
echo(c);