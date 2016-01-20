echo([ for (a = [0,1,2]) if (a == 1) "-" else "+" ]);
echo([ for (a = [0,1,2]) if (a > 0) if (a == 1) "A" else "B" ]);
echo([ for (a = [0,1,2]) if (a > 0) if (a == 1) "A" else "B" else "-" ]);
echo([ for (a = [0 : 3]) if (a < 2) if (a < 1) ["+", a] else ["-", a] ]);
echo([ for (a = [0 : 3]) if (a < 2) ( if (a < 1) ["+", a] ) else ["-", a] ]);
echo([ for (a = [2 : 4]) each [ a, a*a ] ]);
function f() = [ for (a = [0 : 4]) pow(2, a) ];
echo([ each ["a", "b"], each [-5 : -2 : -9], each f(), each "c", each 42, each true ]);
