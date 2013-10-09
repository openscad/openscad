/*
	$parent_modules should return the number of module in the module 
    instantiation stack. The stack is independent on where the modules 
    are defined. It's where they're instantiated that counts.

    parent_module(N) returns the Nth module name in the stack
*/
module print(name) {
  echo("name: ", name);
  for (i=[1:$parent_modules-1]) echo(parent_module(i));
}

module yyy() {
  print("yyy");
}

module test() {
  module xxx() {
    print("xxx");
    yyy();
  }
  print("test");
  xxx();
}

test();
