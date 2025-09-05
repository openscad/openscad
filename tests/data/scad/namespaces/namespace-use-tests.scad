namespace banana {
  use<libxy.scad>
  function xb() = x();
  module yb() { y(); }
}
echo(banana::xb());
banana::yb();
echo(banana::x()); // Should warn
banana::y(); // Should warn
