// Menger Sponge

// Size of edge of sponge
D=100;
// Fractal depth (number of iterations)
n=3;

echo(version=version());

module menger() {
  difference() {
    cube(D, center=true);
    for (v=[[0,0,0], [0,0,90], [0,90,0]])
      rotate(v) menger_negative(side=D, maxside=D, level=n);
  }
}

module menger_negative(side=1, maxside=1, level=1) {
  l=side/3;
  cube([maxside*1.1, l, l], center=true);
  if (level > 1) {
    for (i=[-1:1], j=[-1:1])
      if (i || j)
        translate([0, i*l, j*l])
          menger_negative(side=l, maxside=maxside, level=level-1);
  }
}

difference() {
  rotate([45, atan(1/sqrt(2)), 0]) menger();
  translate([0,0,-D]) cube(2*D, center=true);
}

// Written by Nathan Hellweg, Emmett Lalish and Marius Kintel May 13, 2013
//
// To the extent possible under law, the author(s) have dedicated all
// copyright and related and neighboring rights to this software to the
// public domain worldwide. This software is distributed without any
// warranty.
//
// You should have received a copy of the CC0 Public Domain
// Dedication along with this software.
// If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
