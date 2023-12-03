/*
Instantiate some extrusions in various poses using ModuleLiteral feature - simplification 2
*/
use <MCAD/regular_shapes.scad>
use <MCAD/boxes.scad>

extrusionInfo = [
   [[0,0,0],[0,0,0],module(height) { 
      translate([-3.5,-3.5,0]) cube([7,7,height]);
   }],
   [[10,10,0],[22.5,5,0], module(height) { cylinder(d = 7, h = height, $fn = 30);}],
   [[20,20,5],[45,10,0], module(height) { triangle_prism(height,5);}],
   [[30,30,10],[67.5,15,0], module(height) {octagon_prism(height,4);}],
   [[40,40,15],[90,20,0], module(height) {
      translate([-3.5,-3.5,0]) roundedCube([7,7, height],2,false,false);
   }]
];

for(elem = extrusionInfo)
   translate(elem[0])
      rotate(elem[1]){
         height = 40;
         (elem[2])(height);
      }
