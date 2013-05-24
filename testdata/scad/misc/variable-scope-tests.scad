echo("special variable inheritance");
module special_module(a) {
  echo(a, $fn);
  special_module2(a);
}

module special_module2(b) {
  echo(a);
  echo(b, $fn);
}

special_module(23, $fn=5);

echo("inner variables shadows parameter");
module inner_variables(a, b) {
  b = 24;
  echo(a, b);
}

inner_variables(5, 6);

echo("user-defined special variables as parameter");
module user_defined_special($b) {
  echo($b);
  user_defined_special2();
}

module user_defined_special2() {
  echo($b);
}

user_defined_special(7);

echo("assign only visible in children's scope");
module assigning() {
  echo(c);
}

module assigning2(c) {
  echo(c);
}

assign(c=5) {
  assigning();
  assigning2(c);
}

echo("undeclared variable can still be passed and used");
module undeclared_var() {
  echo(d);
}

undeclared_var(d=6);
