namespace banana {
  c = "yellow";
  function x() = 7;
  module m() {cube(3); }
}
namespace apple {
  function x() = 9;
}
echo(banana::x());
echo(apple::x());
banana::m();
echo(banana::c);
