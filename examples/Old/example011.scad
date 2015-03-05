echo(version=version());

polyhedron(
  points = [
    [10, 0, 0],
    [0, 10, 0],
    [-10, 0, 0],
    [0, -10, 0],
    [0, 0, 10]
  ],
  triangles = [
    [0, 1, 2, 3],
    [4, 1, 0],
    [4, 2, 1],
    [4, 3, 2],
    [4, 0, 3]
  ]
);

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
