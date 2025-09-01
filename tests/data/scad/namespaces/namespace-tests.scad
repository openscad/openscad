y=9;
namespace banana {
  y=7;
  function x() = y;
  module mycube(s) {
    translate([5,0,0]) cube(s);
  }
}
echo(banana::x());
echo(banana::y);
banana::mycube(3);
cube(2);
