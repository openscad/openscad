
module step(len, mod)
{
  for (i = [0:$children-1]) {
    translate([ len*(i - ($children-1)/2), 0, 0 ]) children((i+mod) % $children);
  }
}

echo(version=version());

for (i = [1:4]) {
  translate([0, -250+i*100, 0]) step(100, i) {
    sphere(30);
    cube(60, true);
    cylinder(r = 30, h = 50, center = true);
  
    union() {
      cube(45, true);
      rotate([45, 0, 0]) cube(50, true);
      rotate([0, 45, 0]) cube(50, true);
      rotate([0, 0, 45]) cube(50, true);
    }
  }
}

// Written by Clifford Wolf <clifford@clifford.at> and Marius
// Kintel <marius@kintel.net>
//
// To the extent possible under law, the author(s) have dedicated all
// copyright and related and neighboring rights to this software to the
// public domain worldwide. This software is distributed without any
// warranty.
//
// You should have received a copy of the CC0 Public Domain
// Dedication along with this software.
// If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
