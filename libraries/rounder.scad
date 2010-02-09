// Library: rounder.scad
// Version: 1.1
// Author: Marius Kintel (Idea by Zach Hoeken)
// Copyright: 2010
// License: BSD

// EXAMPLE USAGE:
// roundedBox(20, 30, 40, 5);

// size is a vector [w, h, d]
module roundedBox(size, radius)
{
  cube([size[0], size[1]-radius*2, size[2]-radius*2], center=true);
  cube([size[0]-radius*2, size[1], size[2]-radius*2], center=true);
  cube([size[0]-radius*2, size[1]-radius*2, size[2]], center=true);

  rot = [ [0,0,0], [90,0,90], [90,90,0] ];
  for (axis = [0:2]) {
    for (x = [radius-size[axis]/2, -radius+size[axis]/2],
         y = [radius-size[(axis+1)%3]/2, -radius+size[(axis+1)%3]/2]) {
      rotate(rot[axis]) 
        translate([x,y,0]) 
          cylinder(h=size[(axis+2)%3]-2*radius, r=radius, center=true);
    }
  }
  for (x = [radius-size[0]/2, -radius+size[0]/2],
       y = [radius-size[1]/2, -radius+size[1]/2],
       z = [radius-size[2]/2, -radius+size[2]/2]) {
    translate([x,y,z]) sphere(radius);
  }
}
