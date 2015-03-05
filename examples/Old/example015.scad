
module shape()
{
  difference() {
    translate([ -35, -35 ]) intersection() {
      union() {
        difference() {
          square(100, true);
          square(50, true);
        }
        translate([ 50, 50 ]) square(15, true);
      }
      rotate(45) translate([ 0, -15 ]) square([ 100, 30 ]);
    }
  
    rotate(-45) scale([ 0.7, 1.3 ]) circle(5);
  }
  
  import(file = "example009.dxf", layer = "body", convexity = 6, scale=2);
}

echo(version=version());

// linear_extrude(convexity = 10, center = true)
shape();

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
