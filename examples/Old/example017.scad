
// To render the DXF file from the command line:
// openscad -o example017.dxf -D'mode="parts"' example017.scad

//Mode can be either "parts",  "exploded" or "assembled".
mode = "assembled"; // ["parts",  "exploded", "assembled"]

thickness = 6;
locklen1 = 15;
locklen2 = 10;
boltlen = 15;
midhole = 10;
inner1_to_inner2 = 50;
total_height = 80;

module shape_tripod()
{
  x1 = 0;
  x2 = x1 + thickness;
  x3 = x2 + locklen1;
  x4 = x3 + thickness;
  x5 = x4 + inner1_to_inner2;
  x6 = x5 - thickness;
  x7 = x6 - locklen2;
  x8 = x7 - thickness;
  x9 = x8 - thickness;
  x10 = x9 - thickness;
  
  y1 = 0;
  y2 = y1 + thickness;
  y3 = y2 + thickness;
  y4 = y3 + thickness;
  y5 = y3 + total_height - 3*thickness;
  y6 = y5 + thickness;
  
  union() {
    difference() {
      polygon([
        [ x1, y2 ], [ x2, y2 ],
        [ x2, y1 ], [ x3, y1 ], [ x3, y2 ],
        [ x4, y2 ], [ x4, y1 ], [ x5, y1 ],
        [ x5 + thickness, y3 ], [ x5, y4 ],
        [ x5, y5 ],
        [ x6, y5 ], [ x6, y6 ], [ x7, y6 ], [ x7, y5 ], [ x8, y5 ],
        [ x8, y6 ], [ x9, y5 ],
        [ x9, y4 ], [ x10, y3 ],
        [ x2, y3 ]
      ]);
      translate([ x10, y4 ]) circle(thickness);
      translate([ x5 + thickness, y4 ]) circle(thickness);
    }
  
    translate([ x5, y1 ]) square([ boltlen - thickness, thickness*2 ]);
  
    translate([ x5 + boltlen - thickness, y2 ]) circle(thickness);
  
    translate([ x2, y2 ]) intersection() {
      circle(thickness);
      translate([ -thickness*2, 0 ]) square(thickness*2);
    }
  
    translate([ x8, y5 ]) intersection() {
      circle(thickness);
      translate([ -thickness*2, 0 ]) square(thickness*2);
    }
  }
}

module shape_inner_disc()
{
  difference() {
    circle(midhole + boltlen + 2*thickness + locklen2);
    for (alpha = [ 0, 120, 240 ]) {
      rotate(alpha) translate([ 0, midhole + boltlen + thickness + locklen2/2 ]) square([ thickness, locklen2 ], true);
    }
    circle(midhole + boltlen);
  }
}

module shape_outer_disc()
{
  difference() {
    circle(midhole + boltlen + inner1_to_inner2 + 2*thickness + locklen1);
    for (alpha = [ 0, 120, 240 ]) {
      rotate(alpha) translate([ 0, midhole + boltlen + inner1_to_inner2 + thickness + locklen1/2 ]) square([ thickness, locklen1 ], true);
    }
    circle(midhole + boltlen + inner1_to_inner2);
  }
}

module parts()
{
  tripod_x_off = locklen1 - locklen2 + inner1_to_inner2;
  tripod_y_off = max(midhole + boltlen + inner1_to_inner2 + 4*thickness + locklen1, total_height);

  shape_inner_disc();
  shape_outer_disc();

  for (s = [ [1,1], [-1,1], [1,-1] ]) {
    scale(s) translate([ tripod_x_off, -tripod_y_off ]) shape_tripod();
  }
}

module exploded()
{
  translate([ 0, 0, total_height + 2*thickness ]) linear_extrude(height = thickness, convexity = 4) shape_inner_disc();
  linear_extrude(height = thickness, convexity = 4) shape_outer_disc();

  color([ 0.7, 0.7, 1 ]) for (alpha = [ 0, 120, 240 ]) {
    rotate(alpha)
      translate([ 0, thickness*2 + locklen1 + inner1_to_inner2 + boltlen + midhole, 1.5*thickness ])
        rotate([ 90, 0, -90 ])
          linear_extrude(height = thickness, convexity = 10, center = true) shape_tripod();
  }
}

module bottle()
{
  r = boltlen + midhole;
  h = total_height - thickness*2;

  rotate_extrude(convexity = 2) {
    square([ r, h ]);

    translate([ 0, h ]) {
      intersection() {
        square([ r, r ]);
        scale([ 1, 0.7 ]) circle(r);
      }
    }
    translate([ 0, h+r ]) {
      intersection() {
        translate([ 0, -r/2 ]) square([ r/2, r ]);
        circle(r/2);
      }
    }
  }
}

module assembled()
{
  translate([ 0, 0, total_height - thickness ]) linear_extrude(height = thickness, convexity = 4) shape_inner_disc();
  linear_extrude(height = thickness, convexity = 4) shape_outer_disc();

  color([ 0.7, 0.7, 1 ]) for (alpha = [ 0, 120, 240 ]) {
    rotate(alpha)
      translate([ 0, thickness*2 + locklen1 + inner1_to_inner2 + boltlen + midhole, 0 ])
        rotate([ 90, 0, -90 ])
          linear_extrude(height = thickness, convexity = 10, center = true) shape_tripod();
  }

  % translate([ 0, 0, thickness*2]) bottle();
}

echo(version=version());

if (mode == "parts") parts();

if (mode == "exploded") exploded();

if (mode == "assembled") assembled();

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
