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