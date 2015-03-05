
module example003()
{
  difference() {
    union() {
      cube([30, 30, 30], center = true);
      cube([40, 15, 15], center = true);
      cube([15, 40, 15], center = true);
      cube([15, 15, 40], center = true);
    }
    union() {
      cube([50, 10, 10], center = true);
      cube([10, 50, 10], center = true);
      cube([10, 10, 50], center = true);
    }
  }
}

echo(version=version());

example003();

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
