echo(version=version());

difference() {
  intersection() {
    translate([ -25, -25, -25])
      linear_extrude(height = 50, convexity = 3)
        import(file = "example008.dxf", layer = "G");
    
    rotate(90, [1, 0, 0])
      translate([ -25, -125, -25])
        linear_extrude(height = 50, convexity = 3)
          import(file = "example008.dxf", layer = "E");
    
    rotate(90, [0, 1, 0])
      translate([ -125, -125, -25])
        linear_extrude(height = 50, convexity = 3)
          import(file = "example008.dxf", layer = "B");
  }

  intersection() {
    translate([ -125, -25, -26])
      linear_extrude(height = 52, convexity = 1)
        import(file = "example008.dxf", layer = "X");

    rotate(90, [0, 1, 0])
      translate([ -125, -25, -26])
        linear_extrude(height = 52, convexity = 1)
          import(file = "example008.dxf", layer = "X");
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
