echo("resulting in true:");
echo(is_string(""));
echo(is_string("test"));
echo("resulting in false:");
echo(is_string(0.1));
echo(is_string(1));
echo(is_string(10));
echo(is_string([]));
echo(is_string([1]));
echo(is_string(false));
echo(is_string(0/0)); //nan
echo(is_string((1/0)/(1/0)));  //nan
echo(is_string(1/0));  //inf
echo(is_string(-1/0));  //-inf
echo(is_string(undef)); 
echo("resulting in warnings:");
echo(is_string(1,2,3)); 
echo(is_string()); 