echo("resulting in true:");
echo(is_bool(true));
echo(is_bool(false));
echo("resulting in false:");
echo(is_bool([]));
echo(is_bool([1]));
echo(is_bool("test"));
echo(is_bool(0.1));
echo(is_bool(1));
echo(is_bool(10));
echo(is_bool(0/0)); //nan
echo(is_bool((1/0)/(1/0)));  //nan
echo(is_bool(1/0));  //inf
echo(is_bool(-1/0));  //-inf
echo(is_bool(undef)); 
echo("resulting in warnings:");
echo(is_bool(1,2,3)); 
echo(is_bool()); 