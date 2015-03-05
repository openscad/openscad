// polygon_areas.scad: Another recursion example 

// Draw all geometry
translate([0,20]) color("Red") text("Areas:", size=8, halign="center");
translate([-44,0]) shapeWithArea(3, 10);
translate([-22,0]) shapeWithArea(4, 10);
translate([0,0]) shapeWithArea(6, 10);
translate([22,0]) shapeWithArea(10, 10);
translate([44,0]) shapeWithArea(360, 10);

// One shape with corresponding text
module shapeWithArea(num, r) {
    polygon(ngon(num, r));
    translate([0,-20]) 
        color("Cyan") 
            text(str(round(area(ngon(num, r)))), halign="center", size=8);
}

// Simple list comprehension for creating N-gon vertices
function ngon(num, r) = 
  [for (i=[0:num-1], a=i*360/num) [ r*cos(a), r*sin(a) ]];

// Area of a triangle with the 3rd vertex in the origin
function triarea(v0, v1) = cross(v0, v1) / 2;

// Area of a polygon using the Shoelace formula
function area(vertices) =
  let (areas = [let (num=len(vertices))
                  for (i=[0:num-1]) 
                    triarea(vertices[i], vertices[(i+1)%num])
               ])
      sum(areas);

// Recursive helper function: Sums all values in a list.
// In this case, sum all partial areas into the final area.
function sum(values,s=0) =
  s == len(values) - 1 ? values[s] : values[s] + sum(values,s+1);


echo(version=version());
// Written in 2015 by Marius Kintel <marius@kintel.net>
//
// To the extent possible under law, the author(s) have dedicated all
// copyright and related and neighboring rights to this software to the
// public domain worldwide. This software is distributed without any
// warranty.
//
// You should have received a copy of the CC0 Public Domain
// Dedication along with this software.
// If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
