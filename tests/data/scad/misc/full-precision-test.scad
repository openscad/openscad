/*
   To set the default OpenSCAD number output precision:
   
    • When running from the GUI set 
      Edit → Preferences → Advanced → Displayed number precision → Global
      to the desired default precision.
    
    • When outputting to a file from the command line pass the 
      -O advanced/numberOutputPrecision=N
      option with N set to the desired default precision.  Valid values are 
      between 0 and 17, inclusive (0 uses the built-in default, currently 6).

   To set the precision used when displaying AST dumps:
   
    • When running from the GUI set 
      Edit → Preferences → Advanced → Displayed number precision → AST
      to the desired precision for AST dumps.
    
    • When outputting the AST from the command line pass the same
      -O advanced/numberOutputPrecision=N
      option with N set to the desired default precision.  Valid values are 
      between 0 and 17, inclusive (0 uses the built-in default, currently 6).

*/

/* Setting precision for a running script

   This can be tested, e.g., with the following command line:

./build/openscad --enable object-function \
                 -O advanced/numberOutputPrecision=3 \
                 tests/data/scad/misc/full-precision-test.scad \
                 -o /dev/null \
                 --export-format png \
                 2>&1 | grep ECHO

   The correct output (ECHO statements only) when the default OpenSCAD number 
   output precision is set to 3 (and the object-function feature is enabled) is:

ECHO: "Top-level precision: $fp = 3"
ECHO:
ECHO: "Default precision (currently set to 3)"
ECHO: 1.23e+9, 1.23e+8, 1.23e+18
ECHO: a = 1.23e+9
ECHO: b = 1.23e+8
ECHO: c = 1.23e+18
ECHO: d = 0.1
ECHO: v = [1.23e+9, 7, "fnord", -330, 1.23e+8]
ECHO: r = [1.11 : 5.56e+10 : 20.2]
ECHO: f = function(a, b, c) ((a * 5.56e+4) + pow(b, c))
ECHO: o = { a = 1; b = "hello"; c = true; d = 1.23e+9; }
ECHO: "PI = 3.14"
ECHO: "PI - 123456789.987654321 = -1.23e+8"
ECHO:
ECHO: "Explicit OpenSCAD default precision (0 ⇒ 6)"
ECHO: 1.23451e+9, 1.23457e+8, 1.23457e+18
ECHO: a = 1.23451e+9
ECHO: b = 1.23457e+8
ECHO: c = 1.23457e+18
ECHO: d = 0.1
ECHO: v = [1.23451e+9, 7, "fnord", -330.303, 1.23457e+8]
ECHO: r = [1.11111 : 5.55556e+10 : 20.202]
ECHO: f = function(a, b, c) ((a * 55555.6) + pow(b, c))
ECHO: o = { a = 1; b = "hello"; c = true; d = 1.23451e+9; }
ECHO: "PI = 3.14159"
ECHO: "PI - 123456789.987654321 = -1.23457e+8"
ECHO:
ECHO: "Explicit minimum precision (1)"
ECHO: 1e+9, 1e+8, 1e+18
ECHO: a = 1e+9
ECHO: b = 1e+8
ECHO: c = 1e+18
ECHO: d = 0.1
ECHO: v = [1e+9, 7, "fnord", -3e+2, 1e+8]
ECHO: r = [1 : 6e+10 : 2e+1]
ECHO: f = function(a, b, c) ((a * 6e+4) + pow(b, c))
ECHO: o = { a = 1; b = "hello"; c = true; d = 1e+9; }
ECHO: "PI = 3"
ECHO: "PI - 123456789.987654321 = -1e+8"
ECHO:
ECHO: "Explicit full precision (17)"
ECHO: 1234512345.0987611, 123456789.98765433, 1.2345678998765432e+18
ECHO: a = 1234512345.0987611
ECHO: b = 123456789.98765433
ECHO: c = 1.2345678998765432e+18
ECHO: d = 0.10000000000000001
ECHO: v = [1234512345.678968, 7, "fnord", -330.30285174641023, 123456789.98765433]
ECHO: r = [1.1111111111111112 : 55555555555.555557 : 20.202020202020201]
ECHO: f = function(a, b, c) ((a * 55555.555549999997) + pow(b, c))
ECHO: o = { a = 1; b = "hello"; c = true; d = 1234512345.0987611; }
ECHO: "PI = 3.1415926535897931"
ECHO: "PI - 123456789.987654321 = -123456786.84606168"
ECHO:
ECHO: "Explicit "pretty" precision (16)"
ECHO: 1234512345.098761, 123456789.9876543, 1.234567899876543e+18
ECHO: a = 1234512345.098761
ECHO: b = 123456789.9876543
ECHO: c = 1.234567899876543e+18
ECHO: d = 0.1
ECHO: v = [1234512345.678968, 7, "fnord", -330.3028517464102, 123456789.9876543]
ECHO: r = [1.111111111111111 : 55555555555.55556 : 20.2020202020202]
ECHO: f = function(a, b, c) ((a * 55555.55555) + pow(b, c))
ECHO: o = { a = 1; b = "hello"; c = true; d = 1234512345.098761; }
ECHO: "PI = 3.141592653589793"
ECHO: "PI - 123456789.987654321 = -123456786.8460617"
ECHO:
ECHO: "Mixed precision within a single level"
ECHO: 1.11e+9, "2.2e+9", "3333333333.3333335", "4.44444e+9"
ECHO: 1.111e+9, "2.2e+9", "3333333333.3333335", "4.44444e+9"
ECHO:
ECHO: "Mixed precisions within evaluation tree"
ECHO: 1e+9, "1.23451e+9", "1.2e+9 1.235e+9 1234512345.0987611 1e+9 1.23451e+9"

*/

/* Setting precision for AST output

   This can be tested, e.g., with the following command line:

./build/openscad --enable object-function \
                 -O advanced/numberOutputPrecision=3 \
                 tests/data/scad/misc/full-precision-test.scad \
                 -o - \
                 --export-format ast \
                 2>/dev/null

   The expected AST output when the default OpenSCAD number output precision is 
   set to 3 is:

module simple_tests() {
        echo(a, b, c);
        echo(a = a);
        echo(b = b);
        echo(c = c);
        echo(d = d);
        echo(v = v);
        echo(r = r);
        echo(f = f);
        echo(o = o);
        echo(str("PI = ", PI));
        echo(str("PI - 123456789.987654321 = ", (PI - 1.23e+8)));
}
//Parameter("")
a = 1.23e+9;
//Parameter("")
b = 1.23e+8;
//Parameter("")
c = 1.23e+18;
//Parameter("")
d = 0.1;
//Parameter("")
i = 5.56e+10;
v = [1.23e+9, 7, "fnord", (PI - 333), b];
r = [1.11 : i : 20.2];
f = function(a, b, c) ((a * 5.56e+4) + pow(b, c));
o = object(a = 1, b = "hello", c = true, d = a);
echo(str("Top-level precision: $fp = ", $fp));
echo();
echo(str("Default precision (currently set to ", $fp, ")"));
simple_tests();
echo();
echo("Explicit OpenSCAD default precision (0 ⇒ 6)");
union() {
        $fp = 0;
        simple_tests();
}
echo();
echo("Explicit minimum precision (1)");
union() {
        $fp = 1;
        simple_tests();
}
echo();
echo("Explicit full precision (17)");
union() {
        $fp = 17;
        simple_tests();
}
echo();
echo("Explicit \"pretty\" precision (16)");
union() {
        $fp = 16;
        simple_tests();
}
echo();
echo("Mixed precision within a single level") echo(1.11e+9, let($fp = 2) str(2.22e+9), let($fp = 17) str(3.33e+9), let($fp = 0) str(4.44e+9));
let($fp = 4) echo(1.11e+9, let($fp = 2) str(2.22e+9), let($fp = 17) str(3.33e+9), let($fp = 0) str(4.44e+9));
echo();
echo("Mixed precisions within evaluation tree");
let($fp = 1) echo(a, let($fp = 0) str(a), str(let($fp = 2) str(a), " ", let($fp = 4) str(a, " ", let($fp = 17) str(a)), " ", a, " ", let($fp = 0) str(a)));

*/

//$fp=2;  // Uncomment this line to test override of top-level precision setting

a=1234512345.098760987609;
b=123456789.987654321;
c=123456789.987654321e10;
d=0.1;
i=55555555555.5555555555;
v=[ 1234512345.67896789, 7, "fnord", PI-333.4444444, b ];
r=[ 1.11111111111111111111:i:20.20202020202020202020 ];
f=function (a, b, c) a * 55555.55555 + pow(b, c);
o = object(a = 1, b = "hello", c = true, d=a);


module simple_tests() {
  echo( a, b, c );
  echo( a=a );
  echo( b=b );
  echo( c=c );
  echo( d=d );
  echo( v=v );
  echo( r=r );
  echo( f=f );
  echo( o=o );
  
  echo( str( "PI = ", PI ) );
  echo( str( "PI - 123456789.987654321 = ", PI - 123456789.987654321 ) );
}

echo( str( "Top-level precision: $fp = ", $fp )  );

echo();

echo( str("Default precision (currently set to ", $fp, ")") );
simple_tests();

echo();

echo( "Explicit OpenSCAD default precision (0 ⇒ 6)" );
union() {
  $fp = 0;
  simple_tests();
}

echo();

echo( "Explicit minimum precision (1)" );
union() {
  $fp = 1;
  simple_tests();
}

echo();

echo( "Explicit full precision (17)" );
union() {
  $fp = 17;
  simple_tests();
}

echo();

echo( "Explicit \"pretty\" precision (16)" );
union() {
  $fp = 16;
  simple_tests();
}

echo();

echo( "Mixed precision within a single level" )
echo( 1111111111.1111111111, let($fp=2) str(2222222222.2222222222), let($fp=17) str(3333333333.3333333333), let($fp=0) str(4444444444.4444444444) );
let($fp=4)
  echo( 1111111111.1111111111, let($fp=2) str(2222222222.2222222222), let($fp=17) str(3333333333.3333333333), let($fp=0) str(4444444444.4444444444) );

echo();

echo( "Mixed precisions within evaluation tree" );
let($fp=1) 
    echo( a, 
          let($fp=0)
              str(a), 
          str( 
               let($fp=2) 
                   str(a), 
               " ",
               let($fp=4) 
                   str( 
                        a,
                        " ",
                        let($fp=17)
                            str(a)
                      ), 
               " ",
               a, 
               " ",
               let($fp=0) 
                   str(a) 
             ) 
        );
