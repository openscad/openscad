
// example010.dat generated using octave:
//   d = (sin(1:0.2:10)' * cos(1:0.2:10)) * 10;
//   save("example010.dat", "d");

echo(version=version());

intersection() {
  surface(file = "example010.dat", center = true, convexity = 5);
  
  rotate(45, [0, 0, 1])
    surface(file = "example010.dat", center = true, convexity = 5);
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
