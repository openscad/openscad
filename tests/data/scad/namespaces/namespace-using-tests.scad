using banana;
namespace banana {
  c = "yellow";
  function x() = 7;
  module m() {}
}
namespace apple {
  using banana;
  function z() = 9;
}
echo(x());
m();
echo(z());
echo(c);
echo(apple::c);
