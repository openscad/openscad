echo("Normal variables");
echo(is_undef(a)); //no warning
b="hallo";
echo(is_undef(b));
c=undef;
echo(is_undef(c));

echo(a); //warns
echo(b);
echo(c);

echo("Test with scopes");
if(true){
    echo(is_undef(b));
    
    d=true;
    echo(is_undef(c));
    echo(d);
}
echo(is_undef(d));
echo(d);

echo("Special variables");
echo(is_undef($a)); //no warning
$b=132465;
echo(is_undef($b));
$c=undef;
echo(is_undef($c));

echo($a);
echo($b);
echo($c);

echo("constants resulting in true");
echo(is_undef(undef));

echo("constants resulting in false");
echo(is_undef("Test"));
echo(is_undef(123456));

echo("constants resulting in undef");
echo(is_undef());
echo(is_undef("Test",123));
echo(is_undef(123,456));

echo("functions resulting in true");
echo(is_undef(len("test","upps")));
echo(is_undef(is_undef()));

echo("functions resulting in false");
echo(is_undef(sin(1)));
echo(is_undef(len("test")));
