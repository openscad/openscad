echo("union scope");
a = 4; 
union() { 
  a = 5; 
  echo("local a (5):", a); 
} 
echo("global a (4):", a);


echo("module scope:");
module mymodule(b=6) { 
  b = 7; 
  echo("local b (7)", b); 
} 
mymodule(); 
mymodule(8); 


echo("module children scope:");
module mymodule2(b2=6) { 
  b2 = 2; 
  children(0);
} 
mymodule2(b2=7) {
    b2 = 3;
    echo("b2 (3)", b2);
}

echo("for loop (c = 0,1,25):");
for (i=[0:2]) { 
  c = (i > 1) ? i + 23 : i; 
  echo("c", c); 
}

echo("if scope:");
if (true) {
    d = 8;
    echo("d (8)", d);
}

echo("else scope:");
if (false) {
} else {
    d = 9;
    echo("d (9)", d);
}

echo("anonymous inner scope (scope ignored):");
union() {
    e = 2;
    echo("outer e (3)", e);
    {
        e = 3;
        echo("inner e (3)", e);
    }
}

echo("anonymous scope (scope ignored):"); 
f=1; 
echo("outer f (2)", f); 
{ 
    f=2; 
    echo("inner f (2)", f); 
}

echo("anonymous scope reassign:");
{ 
    g=1; 
    echo("g (2)", g);
    g=2; 
}

echo("anonymous reassign using outer (scope ignored)", h); 
h=5; 
{ 
    h=h*2; // Not allowed
    echo("h (undef)", h); 
} 

echo("override variable in assign scope:");
assign(i=9) {
    i=10;
    echo("i (10)", i);
}

