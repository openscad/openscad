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

module box(w,h,d) {
  cube([w,h,d], true);
}

module roundedBox(w,h,d,radius) {
  size = [w,h,d];
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

module oval(w,h,d, center = false) {
  scale([1, h/w, 1]) cylinder(h=d, r=w, center=center);
}

module tube(height, radius, wall, center = false) {
  difference() {
    cylinder(h=height, r=radius, center=center);
    cylinder(h=height, r=radius-wall, center=center);
  }
}

module ovalTube(height, rx, ry, wall) {
  difference() {
    scale([1, ry/rx, 1]) cylinder(h=height, r=rx);
    scale([(rx-wall)/rx, (ry-wall)/rx, 1]) cylinder(h=height, r=rx);
  }
}

module hexagon(size, depth) {
  boxWidth = size/1.75;
  for (r = [-60, 0, 60]) rotate([0,0,r]) cube([boxWidth, size, depth], true);
}

module octagon(size, depth) {
  intersection() {
    cube([size, size, depth], true);
    rotate([0,0,45]) cube([size, size, depth], true);
  }
}

module dodecagon(size, depth) {
  intersection() {
    hexagon(size, depth);
    rotate([0,0,90]) hexagon(size, depth);
  }
}

module hexagram(size, depth) {
  boxWidth=size/1.75;
  for (v = [[0,1],[0,-1],[1,-1]]) {
    intersection() {
      rotate([0,0,60*v[0]]) cube([size, boxWidth, depth], true);
      rotate([0,0,60*v[1]]) cube([size, boxWidth, depth], true);
    }
  }
}

module rightTriangle(adjacent, opposite, depth) {
  difference() {
    translate([-adjacent/2,opposite/2,0]) box(adjacent, opposite, depth);
    translate([-adjacent,0,0]) {
      rotate([0,0,atan(opposite/adjacent)]) dislocateBox(adjacent*2, opposite, depth);
    }
  }
}

module equiTriangle(side, depth) {
  difference() {
    translate([-side/2,side/2,0]) box(side, side, depth);
    rotate([0,0,30]) dislocateBox(side*2, side, depth);
    translate([-side,0,0]) {
      rotate([0,0,60]) dislocateBox(side*2, side, depth);
    }
  }
}

module 12ptStar(size, depth) {
  starNum=3;
  starAngle=360/starNum;
  for (s=[1:starNum]) {
    rotate([0, 0, s*starAngle]) box(size, size, depth);
  }
}

//-----------------------
//MOVES THE ROTATION AXIS OF A BOX FROM ITS CENTER TO THE BOTTOM LEFT CORNER
//FIXME: Why are the dimensions changed?
// why not just translate([0,0,-d/2]) cube([w,h,d]);
module dislocateBox(w,h,d) {
  translate([w/2,h,0]) {
    difference() {
      box(w, h*2, d+1);
      translate([-w,0,0]) box(w, h*2, d+1);
    }
  }
}







