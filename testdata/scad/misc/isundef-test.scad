echo(isundef(a)); //no warning
b="hallo";
echo(isundef(b));
c=undef;
echo(isundef(c));

echo(a); //warns
echo(b);
echo(c);

if(true){
    echo(isundef(b));
    
    d=true;
    echo(isundef(c));
    echo(d);
}
echo(isundef(d));
echo(d);

echo(isundef($a)); //no warning
$b=132465;
echo(isundef($b));
$c=undef;
echo(isundef($c));

echo($a);
echo($b);
echo($c);