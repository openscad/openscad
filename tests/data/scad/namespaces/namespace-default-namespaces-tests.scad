namespace banana {
  t = builtins::concat("hello", "world");
  module mycube(s) {
    translate([15,0,0]) builtins::cube(s);
  }
  function x() = top_level::x;
}
x="I'm reading from the top level!";
echo(banana::t);
echo(banana::x());
banana::mycube(9);
cube(6);
