namespace banana {
  t = concat("hello", "world");
  module mycube(s) {
    translate([15,0,0]) cube(s);
  }
}
echo(banana::t);
banana::mycube(9);
cube(6);
