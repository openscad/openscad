module example001()
{
  function r_from_dia(d) = d / 2;

  module rotcy(rot, r, h) {
    rotate(90, rot)
      cylinder(r = r, h = h, center = true);
  }

  difference() {
    sphere(r = r_from_dia(size));
    rotcy([0, 0, 0], cy_r, cy_h);
    rotcy([1, 0, 0], cy_r, cy_h);
    rotcy([0, 1, 0], cy_r, cy_h);
  }

  size = 50;
  hole = 25;

  cy_r = r_from_dia(hole);
  cy_h = r_from_dia(size * 2.5);
}

echo(version=version());

example001();

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
