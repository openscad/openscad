echo(is_undef(a)); //no warning
b="hallo";
echo(is_undef(b));
c=undef;
echo(is_undef(c));

echo(a); //warns
echo(b);
echo(c);

if(true){
    echo(is_undef(b));
    
    d=true;
    echo(is_undef(c));
    echo(d);
}
echo(is_undef(d));
echo(d);

echo(is_undef($a)); //no warning
$b=132465;
echo(is_undef($b));
$c=undef;
echo(is_undef($c));

echo($a);
echo($b);
echo($c);
