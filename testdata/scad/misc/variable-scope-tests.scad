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

echo("$children scope");
module child_module_2() {
  echo("$children should be 4: ", $children);
  for(i=[0:$children-1]) children(i);
}

module child_module_1() {
  echo("$children should be 1: ", $children);
  child_module_2() {
    echo("$children should be 1: ", $children);
    children(0);
    echo("child_module_2 child 0");
    echo("child_module_2 child 1");
  }
}

child_module_1() echo("child_module_1 child");

echo("copy $children");
module copy_children_module() {
  kids = $children;
  echo("copy_children_module: ", kids, $children);
}

copy_children_module() {
  cube();
  sphere();
}

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

echo("attempt to assign from a not-yet-defined variable which also exists globally");

globalval = 1;
// Test that b = a turns into b = 1, heeding the order of the assignments
// See issue #399 for more discussion
module global_lookup() {
  b = globalval; // Should be assigned 1 since the local one isn't yet defined
  globalval = 5; // Overrides the value for the local scope only
  echo(globalval,b); // Should output 5, 1
}
global_lookup();
