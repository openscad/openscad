echo( "0", 0);
echo("-0",-0);

echo(" 0/ 0", 0 / 0);
echo("-0/ 0",-0 / 0);
echo(" 0/-0", 0 /-0);
echo("-0/-0",-0 /-0);

echo(" 1/ 0", 1 / 0);
echo("-1/ 0",-1 / 0);
echo(" 1/-0", 1 /-0);
echo("-1/-0",-1 /-0);

echo(" 0/ 1", 0 / 1);
echo("-0/ 1",-0 / 1);
echo(" 0/-1", 0 /-1);
echo("-0/-1",-0 /-1);


echo(  "sin( 0/ 0)",sin( 0/ 0));
echo(  "sin(-0/ 0)",sin(-0/ 0));
echo(  "sin( 0/-0)",sin( 0/-0));
echo(  "sin(-0/-0)",sin(-0/-0));

echo(  "sin( 1/ 0)",sin( 1/ 0));
echo(  "sin(-1/ 0)",sin(-1/ 0));
echo(  "sin( 1/-0)",sin( 1/-0));
echo(  "sin(-1/-0)",sin(-1/-0));

echo(  "sin( 0/ 1)",sin( 0/ 1));
echo(  "sin(-0/ 1)",sin(-0/ 1));
echo(  "sin( 0/-1)",sin( 0/-1));
echo(  "sin(-0/-1)",sin(-0/-1));

echo(  "cos( 0/ 0)",cos( 0/ 0));
echo(  "cos(-0/ 0)",cos(-0/ 0));
echo(  "cos( 0/-0)",cos( 0/-0));
echo(  "cos(-0/-0)",cos(-0/-0));

echo(  "cos( 1/ 0)",cos( 1/ 0));
echo(  "cos(-1/ 0)",cos(-1/ 0));
echo(  "cos( 1/-0)",cos( 1/-0));
echo(  "cos(-1/-0)",cos(-1/-0));

echo(  "cos( 0/ 1)",cos( 0/ 1));
echo(  "cos(-0/ 1)",cos(-0/ 1));
echo(  "cos( 0/-1)",cos( 0/-1));
echo(  "cos(-0/-1)",cos(-0/-1));


echo(  "tan( 0/ 0)",tan( 0/ 0));
echo(  "tan(-0/ 0)",tan(-0/ 0));
echo(  "tan( 0/-0)",tan( 0/-0));
echo(  "tan(-0/-0)",tan(-0/-0));

echo(  "tan( 1/ 0)",tan( 1/ 0));
echo(  "tan(-1/ 0)",tan(-1/ 0));
echo(  "tan( 1/-0)",tan( 1/-0));
echo(  "tan(-1/-0)",tan(-1/-0));

echo(  "tan( 0/ 1)",tan( 0/ 1));
echo(  "tan(-0/ 1)",tan(-0/ 1));
echo(  "tan( 0/-1)",tan( 0/-1));
echo(  "tan(-0/-1)",tan(-0/-1));



echo(  "asin( 0/ 0)",asin( 0/ 0));
echo(  "asin(-0/ 0)",asin(-0/ 0));
echo(  "asin( 0/-0)",asin( 0/-0));
echo(  "asin(-0/-0)",asin(-0/-0));

echo(  "asin( 1/ 0)",asin( 1/ 0));
echo(  "asin(-1/ 0)",asin(-1/ 0));
echo(  "asin( 1/-0)",asin( 1/-0));
echo(  "asin(-1/-0)",asin(-1/-0));

echo(  "asin( 0/ 1)",asin( 0/ 1));
echo(  "asin(-0/ 1)",asin(-0/ 1));
echo(  "asin( 0/-1)",asin( 0/-1));
echo(  "asin(-0/-1)",asin(-0/-1));

echo(  "acos( 0/ 0)",acos( 0/ 0));
echo(  "acos(-0/ 0)",acos(-0/ 0));
echo(  "acos( 0/-0)",acos( 0/-0));
echo(  "acos(-0/-0)",acos(-0/-0));

echo(  "acos( 1/ 0)",acos( 1/ 0));
echo(  "acos(-1/ 0)",acos(-1/ 0));
echo(  "acos( 1/-0)",acos( 1/-0));
echo(  "acos(-1/-0)",acos(-1/-0));

echo(  "acos( 0/ 1)",acos( 0/ 1));
echo(  "acos(-0/ 1)",acos(-0/ 1));
echo(  "acos( 0/-1)",acos( 0/-1));
echo(  "acos(-0/-1)",acos(-0/-1));


echo(  "atan( 0/ 0)",atan( 0/ 0));
echo(  "atan(-0/ 0)",atan(-0/ 0));
echo(  "atan( 0/-0)",atan( 0/-0));
echo(  "atan(-0/-0)",atan(-0/-0));

echo(  "atan( 1/ 0)",atan( 1/ 0));
echo(  "atan(-1/ 0)",atan(-1/ 0));
echo(  "atan( 1/-0)",atan( 1/-0));
echo(  "atan(-1/-0)",atan(-1/-0));

echo(  "atan( 0/ 1)",atan( 0/ 1));
echo(  "atan(-0/ 1)",atan(-0/ 1));
echo(  "atan( 0/-1)",atan( 0/-1));
echo(  "atan(-0/-1)",atan(-0/-1));

// atan2 has some interesting portability issues
// http://pubs.opengroup.org/onlinepubs/009695399/functions/atan2.html
// http://www.cplusplus.com/reference/cmath/atan2/
// http://msdn.microsoft.com/en-us/library/system.math.atan2.aspx
echo("atan2(  0,   0)",atan2(  0,    0));
echo("atan2( -0,   0)",atan2( -0,    0));
echo("atan2(  0,  -0)",atan2(  0,   -0));
echo("atan2( -0,  -0)",atan2( -0,   -0));

echo("atan2(  0,   1)",atan2(  0,    1));
echo("atan2(  0,  -1)",atan2(  0,   -1));
echo("atan2(  1,   0)",atan2(  1,    0));
echo("atan2( -1,   0)",atan2( -1,    0));

echo("atan2( -0,   1)",atan2( -0,    1));
echo("atan2( -0,  -1)",atan2( -0,   -1));
echo("atan2(  1,  -0)",atan2(  1,   -0));
echo("atan2( -1,  -0)",atan2( -1,   -0));

echo("atan2( 1/0, 1/0)",atan2( 1/0, 1/0));
echo("atan2( 1/0,-1/0)",atan2( 1/0,-1/0));
echo("atan2(-1/0, 1/0)",atan2(-1/0, 1/0));
echo("atan2(-1/0,-1/0)",atan2(-1/0,-1/0));

echo("atan2( 1/0, 1  )",atan2( 1/0, 1  ));
echo("atan2( 1/0,-1  )",atan2( 1/0,-1  ));
echo("atan2(-1/0, 1  )",atan2(-1/0, 1  ));
echo("atan2(-1/0,-1  )",atan2(-1/0,-1  ));
echo("atan2( 1/0, 0  )",atan2( 1/0, 0  ));
echo("atan2( 1/0,-0  )",atan2( 1/0,-0  ));
echo("atan2(-1/0, 0  )",atan2(-1/0, 0  ));
echo("atan2(-1/0,-0  )",atan2(-1/0,-0  ));

echo("atan2( 1  , 1/0)",atan2( 1  , 1/0));
echo("atan2( 1  ,-1/0)",atan2( 1  ,-1/0));
echo("atan2(-1  , 1/0)",atan2(-1  , 1/0));
echo("atan2(-1  ,-1/0)",atan2(-1  ,-1/0));
echo("atan2(   0, 1/0)",atan2(   0, 1/0));
echo("atan2(   0,-1/0)",atan2(   0,-1/0));
echo("atan2(  -0, 1/0)",atan2(  -0, 1/0));
echo("atan2(  -0,-1/0)",atan2(  -0,-1/0));

echo(  "ceil( 0/ 0)",ceil( 0/ 0));
echo(  "ceil(-0/ 0)",ceil(-0/ 0));
echo(  "ceil( 0/-0)",ceil( 0/-0));
echo(  "ceil(-0/-0)",ceil(-0/-0));

echo(  "ceil( 1/ 0)",ceil( 1/ 0));
echo(  "ceil(-1/ 0)",ceil(-1/ 0));
echo(  "ceil( 1/-0)",ceil( 1/-0));
echo(  "ceil(-1/-0)",ceil(-1/-0));

echo(  "ceil( 0/ 1)",ceil( 0/ 1));
echo(  "ceil(-0/ 1)",ceil(-0/ 1));
echo(  "ceil( 0/-1)",ceil( 0/-1));
echo(  "ceil(-0/-1)",ceil(-0/-1));


echo(  "floor( 0/ 0)",floor( 0/ 0));
echo(  "floor(-0/ 0)",floor(-0/ 0));
echo(  "floor( 0/-0)",floor( 0/-0));
echo(  "floor(-0/-0)",floor(-0/-0));

echo(  "floor( 1/ 0)",floor( 1/ 0));
echo(  "floor(-1/ 0)",floor(-1/ 0));
echo(  "floor( 1/-0)",floor( 1/-0));
echo(  "floor(-1/-0)",floor(-1/-0));

echo(  "floor( 0/ 1)",floor( 0/ 1));
echo(  "floor(-0/ 1)",floor(-0/ 1));
echo(  "floor( 0/-1)",floor( 0/-1));
echo(  "floor(-0/-1)",floor(-0/-1));



echo(  "ln( 0/ 0)",ln( 0/ 0));
echo(  "ln(-0/ 0)",ln(-0/ 0));
echo(  "ln( 0/-0)",ln( 0/-0));
echo(  "ln(-0/-0)",ln(-0/-0));

echo(  "ln( 1/ 0)",ln( 1/ 0));
echo(  "ln(-1/ 0)",ln(-1/ 0));
echo(  "ln( 1/-0)",ln( 1/-0));
echo(  "ln(-1/-0)",ln(-1/-0));

echo(  "ln( 0/ 1)",ln( 0/ 1));
echo(  "ln(-0/ 1)",ln(-0/ 1));
echo(  "ln( 0/-1)",ln( 0/-1));
echo(  "ln(-0/-1)",ln(-0/-1));




echo(  "log( 0/ 0)",log( 0/ 0));
echo(  "log(-0/ 0)",log(-0/ 0));
echo(  "log( 0/-0)",log( 0/-0));
echo(  "log(-0/-0)",log(-0/-0));

echo(  "log( 1/ 0)",log( 1/ 0));
echo(  "log(-1/ 0)",log(-1/ 0));
echo(  "log( 1/-0)",log( 1/-0));
echo(  "log(-1/-0)",log(-1/-0));

echo(  "log( 0/ 1)",log( 0/ 1));
echo(  "log(-0/ 1)",log(-0/ 1));
echo(  "log( 0/-1)",log( 0/-1));
echo(  "log(-0/-1)",log(-0/-1));




echo(  "round( 0/ 0)",round( 0/ 0));
echo(  "round(-0/ 0)",round(-0/ 0));
echo(  "round( 0/-0)",round( 0/-0));
echo(  "round(-0/-0)",round(-0/-0));

echo(  "round( 1/ 0)",round( 1/ 0));
echo(  "round(-1/ 0)",round(-1/ 0));
echo(  "round( 1/-0)",round( 1/-0));
echo(  "round(-1/-0)",round(-1/-0));

echo(  "round( 0/ 1)",round( 0/ 1));
echo(  "round(-0/ 1)",round(-0/ 1));
echo(  "round( 0/-1)",round( 0/-1));
echo(  "round(-0/-1)",round(-0/-1));



echo(  "sign( 0/ 0)",sign( 0/ 0));
echo(  "sign(-0/ 0)",sign(-0/ 0));
echo(  "sign( 0/-0)",sign( 0/-0));
echo(  "sign(-0/-0)",sign(-0/-0));

echo(  "sign( 1/ 0)",sign( 1/ 0));
echo(  "sign(-1/ 0)",sign(-1/ 0));
echo(  "sign( 1/-0)",sign( 1/-0));
echo(  "sign(-1/-0)",sign(-1/-0));

echo(  "sign( 0/ 1)",sign( 0/ 1));
echo(  "sign(-0/ 1)",sign(-0/ 1));
echo(  "sign( 0/-1)",sign( 0/-1));
echo(  "sign(-0/-1)",sign(-0/-1));



echo(  "sqrt( 0/ 0)",sqrt( 0/ 0));
echo(  "sqrt(-0/ 0)",sqrt(-0/ 0));
echo(  "sqrt( 0/-0)",sqrt( 0/-0));
echo(  "sqrt(-0/-0)",sqrt(-0/-0));

echo(  "sqrt( 1/ 0)",sqrt( 1/ 0));
echo(  "sqrt(-1/ 0)",sqrt(-1/ 0));
echo(  "sqrt( 1/-0)",sqrt( 1/-0));
echo(  "sqrt(-1/-0)",sqrt(-1/-0));

echo(  "sqrt( 0/ 1)",sqrt( 0/ 1));
echo(  "sqrt(-0/ 1)",sqrt(-0/ 1));
echo(  "sqrt( 0/-1)",sqrt( 0/-1));
echo(  "sqrt(-0/-1)",sqrt(-0/-1));



echo(  "pow(2, 0/ 0)",pow(2, 0/ 0));
echo(  "pow(2,-0/ 0)",pow(2,-0/ 0));
echo(  "pow(2, 0/-0)",pow(2, 0/-0));
echo(  "pow(2,-0/-0)",pow(2,-0/-0));

echo(  "pow(2, 1/ 0)",pow(2, 1/ 0));
echo(  "pow(2,-1/ 0)",pow(2,-1/ 0));
echo(  "pow(2, 1/-0)",pow(2, 1/-0));
echo(  "pow(2,-1/-0)",pow(2,-1/-0));

echo(  "pow(2, 0/ 1)",pow(2, 0/ 1));
echo(  "pow(2,-0/ 1)",pow(2,-0/ 1));
echo(  "pow(2, 0/-1)",pow(2, 0/-1));
echo(  "pow(2,-0/-1)",pow(2,-0/-1));




echo(  "exp(2, 0/ 0)",exp(2, 0/ 0));
echo(  "exp(2,-0/ 0)",exp(2,-0/ 0));
echo(  "exp(2, 0/-0)",exp(2, 0/-0));
echo(  "exp(2,-0/-0)",exp(2,-0/-0));

echo(  "exp(2, 1/ 0)",exp(2, 1/ 0));
echo(  "exp(2,-1/ 0)",exp(2,-1/ 0));
echo(  "exp(2, 1/-0)",exp(2, 1/-0));
echo(  "exp(2,-1/-0)",exp(2,-1/-0));

echo(  "exp(2, 0/ 1)",exp(2, 0/ 1));
echo(  "exp(2,-0/ 1)",exp(2,-0/ 1));
echo(  "exp(2, 0/-1)",exp(2, 0/-1));
echo(  "exp(2,-0/-1)",exp(2,-0/-1));


echo("max(  0,   0)",max(  0,    0));
echo("max( -0,   0)",max( -0,    0));
echo("max(  0,  -0)",max(  0,   -0));
echo("max( -0,  -0)",max( -0,   -0));

echo("max(  0,   1)",max(  0,    1));
echo("max(  0,  -1)",max(  0,   -1));
echo("max(  1,   0)",max(  1,    0));
echo("max( -1,   0)",max( -1,    0));

echo("max( -0,   1)",max( -0,    1));
echo("max( -0,  -1)",max( -0,   -1));
echo("max(  1,  -0)",max(  1,   -0));
echo("max( -1,  -0)",max( -1,   -0));

echo("max( 1/0, 1/0)",max( 1/0, 1/0));
echo("max( 1/0,-1/0)",max( 1/0,-1/0));
echo("max(-1/0, 1/0)",max(-1/0, 1/0));
echo("max(-1/0,-1/0)",max(-1/0,-1/0));

echo("max( 1/0, 1  )",max( 1/0, 1  ));
echo("max( 1/0,-1  )",max( 1/0,-1  ));
echo("max(-1/0, 1  )",max(-1/0, 1  ));
echo("max(-1/0,-1  )",max(-1/0,-1  ));
echo("max( 1/0, 0  )",max( 1/0, 0  ));
echo("max( 1/0,-0  )",max( 1/0,-0  ));
echo("max(-1/0, 0  )",max(-1/0, 0  ));
echo("max(-1/0,-0  )",max(-1/0,-0  ));

echo("max( 1  , 1/0)",max( 1  , 1/0));
echo("max( 1  ,-1/0)",max( 1  ,-1/0));
echo("max(-1  , 1/0)",max(-1  , 1/0));
echo("max(-1  ,-1/0)",max(-1  ,-1/0));
echo("max(   0, 1/0)",max(   0, 1/0));
echo("max(   0,-1/0)",max(   0,-1/0));
echo("max(  -0, 1/0)",max(  -0, 1/0));
echo("max(  -0,-1/0)",max(  -0,-1/0));



echo("min(  0,   0)",min(  0,    0));
echo("min( -0,   0)",min( -0,    0));
echo("min(  0,  -0)",min(  0,   -0));
echo("min( -0,  -0)",min( -0,   -0));

echo("min(  0,   1)",min(  0,    1));
echo("min(  0,  -1)",min(  0,   -1));
echo("min(  1,   0)",min(  1,    0));
echo("min( -1,   0)",min( -1,    0));

echo("min( -0,   1)",min( -0,    1));
echo("min( -0,  -1)",min( -0,   -1));
echo("min(  1,  -0)",min(  1,   -0));
echo("min( -1,  -0)",min( -1,   -0));

echo("min( 1/0, 1/0)",min( 1/0, 1/0));
echo("min( 1/0,-1/0)",min( 1/0,-1/0));
echo("min(-1/0, 1/0)",min(-1/0, 1/0));
echo("min(-1/0,-1/0)",min(-1/0,-1/0));

echo("min( 1/0, 1  )",min( 1/0, 1  ));
echo("min( 1/0,-1  )",min( 1/0,-1  ));
echo("min(-1/0, 1  )",min(-1/0, 1  ));
echo("min(-1/0,-1  )",min(-1/0,-1  ));
echo("min( 1/0, 0  )",min( 1/0, 0  ));
echo("min( 1/0,-0  )",min( 1/0,-0  ));
echo("min(-1/0, 0  )",min(-1/0, 0  ));
echo("min(-1/0,-0  )",min(-1/0,-0  ));

echo("min( 1  , 1/0)",min( 1  , 1/0));
echo("min( 1  ,-1/0)",min( 1  ,-1/0));
echo("min(-1  , 1/0)",min(-1  , 1/0));
echo("min(-1  ,-1/0)",min(-1  ,-1/0));
echo("min(   0, 1/0)",min(   0, 1/0));
echo("min(   0,-1/0)",min(   0,-1/0));
echo("min(  -0, 1/0)",min(  -0, 1/0));
echo("min(  -0,-1/0)",min(  -0,-1/0));
