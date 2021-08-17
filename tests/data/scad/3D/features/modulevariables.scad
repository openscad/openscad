module mymodule(modparam) {
  inner_variable = 23;
  inner_variable2 = modparam * 2;
  cylinder(r1=inner_variable, r2=inner_variable2, h=10);
}

mymodule(5);
