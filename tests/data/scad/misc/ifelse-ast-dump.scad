// This tests many variations of if/else with single/multiple children
// to verify AST dump indentation behavior.
if (true) {
  cube();
  sphere();
  translate([10,10,10]) if (false) {
    cylinder();
    cube();
  } else {
    sphere();
    cube();
  }
} else {
  echo("hi");
}

if (true) cube();
else sphere();

if (true) {
  if (false) {
    if (true) {
      echo("hello");
      echo("world");
    } else {
      if (true) echo("hello"); else echo("bye bye");
      echo("world");
    }
    assert(true);
  } else {
    echo("hello world");
  }
  echo("!");
}
