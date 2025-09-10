using banana;
namespace banana {
  c = "yellow";
  function x() = 7;
  module m() { cube(10,center=true); }
}
namespace apple {
  using banana;
  function z() = 9;
}
echo(x());
m();
echo(z()); // Should trigger warning
echo(c);
echo(apple::c); // Should trigger warning
