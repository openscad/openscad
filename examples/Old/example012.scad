// example012.stl is generated from Basics/LetterBlock.scad

echo(version=version());

difference() {
  sphere(20);
  
  translate([ -2.92, 0.5, +20 ])
    rotate([180, 0, 180])
      import("example012.stl", convexity = 5);
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
