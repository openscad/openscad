
/*
The output of the file should be 2,2,2,4,4,4 as openscad always considers the latest functions/modules definition
*/

echo(f());

function f() = 1;

echo(f());

function f() = 2;

echo(f());

m();

module m() {
  function f() = 3;
  echo(f());
}

m();

module m() {
  function f() = 4;
  echo(f());
}

m();