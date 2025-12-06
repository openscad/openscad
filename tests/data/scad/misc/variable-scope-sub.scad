sub_global = 15;

module submodule() {
  echo($children);
  echo(submodule_var);
  submodule_var = 16;
  module subsubmodule() {
    echo($children);
    subsubmodule_var = 17;
    echo(subsubmodule_var);
    children(0);
  }
  subsubmodule() {children(0); sphere();}
}

module submodule2() {
  echo(sub_global);
  echo($children);
}

module submain() {
  echo(global_var); // Undefined global var
  submodule() {submodule2() sphere(); cube();}
}
