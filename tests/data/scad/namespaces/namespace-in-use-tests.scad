use<lib1.scad>
use<lib2.scad>
echo(x_called_from_banana());
echo(x_from_banana());
// None should be found; apparently need to add warnings
echo(banana::x());
echo(banana::y());
echo(banana::z());
