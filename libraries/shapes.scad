/*
 *  OpenSCAD Shapes Library (www.openscad.org)
 *  Copyright (C) 2009  Catarina Mota
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
*/


//box(width, height, depth);
//roundedBox(width, height, depth, factor);
//cone(height, radius);
//oval(width, height, depth);
//tube(height, radius, wall);
//ovalTube(width, height, depth, wall);
//hexagon(height, depth);
//octagon(height, depth);
//dodecagon(height, depth);
//hexagram(height, depth);
//rightTriangle(adjacent, opposite, depth);
//equiTriangle(side, depth);
//12ptStar(height, depth);

//----------------------

// size is a vector [w, h, d]
module box(size) {
  cube(size, true);
}

// size is a vector [w, h, d]
module roundedBox(size, radius) {
  cube(size - [2*radius,0,0], true);
  cube(size - [0,2*radius,0], true);
  for (x = [radius-size[0]/2, -radius+size[0]/2],
       y = [radius-size[1]/2, -radius+size[1]/2]) {
    translate([x,y,0]) cylinder(r=radius, h=size[2], center=true);
  }
}

module cone(height, radius, center = false) {
  cylinder(height, radius, 0, center);
}

module oval(w,h, height, center = false) {
  scale([1, h/w, 1]) cylinder(h=height, r=w, center=center);
}

// wall is wall thickness
module tube(height, radius, wall, center = false) {
  difference() {
    cylinder(h=height, r=radius, center=center);
    cylinder(h=height, r=radius-wall, center=center);
  }
}

// wall is wall thickness
module ovalTube(height, rx, ry, wall, center = false) {
  difference() {
    scale([1, ry/rx, 1]) cylinder(h=height, r=rx, center=center);
    scale([(rx-wall)/rx, (ry-wall)/rx, 1]) cylinder(h=height, r=rx, center=center);
  }
}

// size is the XY plane size, height in Z
module hexagon(size, height) {
  boxWidth = size/1.75;
  for (r = [-60, 0, 60]) rotate([0,0,r]) cube([boxWidth, size, height], true);
}

// size is the XY plane size, height in Z
module octagon(size, height) {
  intersection() {
    cube([size, size, height], true);
    rotate([0,0,45]) cube([size, size, height], true);
  }
}

// size is the XY plane size, height in Z
module dodecagon(size, height) {
  intersection() {
    hexagon(size, height);
    rotate([0,0,90]) hexagon(size, height);
  }
}

// size is the XY plane size, height in Z
module hexagram(size, height) {
  boxWidth=size/1.75;
  for (v = [[0,1],[0,-1],[1,-1]]) {
    intersection() {
      rotate([0,0,60*v[0]]) cube([size, boxWidth, height], true);
      rotate([0,0,60*v[1]]) cube([size, boxWidth, height], true);
    }
  }
}

module rightTriangle(adjacent, opposite, height) {
  difference() {
    translate([-adjacent/2,opposite/2,0]) cube([adjacent, opposite, height], true);
    translate([-adjacent,0,0]) {
      rotate([0,0,atan(opposite/adjacent)]) dislocateBox(adjacent*2, opposite, height);
    }
  }
}

module equiTriangle(side, height) {
  difference() {
    translate([-side/2,side/2,0]) cube([side, side, height], true);
    rotate([0,0,30]) dislocateBox(side*2, side, height);
    translate([-side,0,0]) {
      rotate([0,0,60]) dislocateBox(side*2, side, height);
    }
  }
}

module 12ptStar(size, height) {
  starNum = 3;
  starAngle = 360/starNum;
  for (s = [1:starNum]) {
    rotate([0, 0, s*starAngle]) cube([size, size, height], true);
  }
}

//-----------------------
//MOVES THE ROTATION AXIS OF A BOX FROM ITS CENTER TO THE BOTTOM LEFT CORNER
//FIXME: Why are the dimensions changed?
// why not just translate([0,0,-d/2]) cube([w,h,d]);
module dislocateBox(w,h,d) {
  translate([w/2,h,0]) {
    difference() {
      cube([w, h*2, d+1]);
      translate([-w,0,0]) cube([w, h*2, d+1]);
    }
  }
}
