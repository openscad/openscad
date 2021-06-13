function foo() = search(undef,undef);

if (false && foo()) {
  echo("Fail");
} else {
  echo("Pass");
}

if (true || foo()) {
  echo("Pass");
} else {
  echo("Fail");
}

if (true && true) {
  echo("Pass");
}

if (false || true) {
  echo("Pass");
}

function ternarytest() = true ? true : foo();

if (ternarytest()) {
  echo("Pass");
}
