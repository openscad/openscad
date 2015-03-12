
module example005()
{
  translate([0, 0, -120]) {
    difference() {
      cylinder(h = 50, r = 100);
      translate([0, 0, 10]) cylinder(h = 50, r = 80);
      translate([100, 0, 35]) cube(50, center = true);
    }
    for (i = [0:5]) {
      echo(360*i/6, sin(360*i/6)*80, cos(360*i/6)*80);
      translate([sin(360*i/6)*80, cos(360*i/6)*80, 0 ])
        cylinder(h = 200, r=10);
    }
    translate([0, 0, 200])
      cylinder(h = 80, r1 = 120, r2 = 0);
  }
}

echo(version=version());

example005();

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
