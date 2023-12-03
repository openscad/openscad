/*
Instantiate some extrusions in various poses using ModuleLiteral feature - simplification 1
*/
use <MCAD/regular_shapes.scad>
use <MCAD/boxes.scad>

// extrusion modules parameterised on height
extSquare = module (height) {
  translate([-3.5,-3.5,0]) cube([7,7,height]);
};
extCircle = module (height) {
  cylinder(d = 7, h = height, $fn = 30);
};
extTriangle = module (height) { triangle_prism(height,5); };
extOctagon = module (height) { octagon_prism(height,4); };
extRoundedCube = module (height) {
   translate([-3.5,-3.5,0]) 
     roundedCube([7,7, height],2,false,false);
 };
/*
using ModuleLiterals feature, we can store Module Literals representing the modules in the same array
*/
// array of transforms
// translations in col 0
// and rotations in col 1
// and module literals in col 3 
extrusionInfo = [
  [[0,0,0],[0,0,0],extSquare],
  [[10,10,0],[22.5,5,0],extCircle],
  [[20,20,5],[45,10,0],extTriangle],
  [[30,30,10],[67.5,15,0], extOctagon],
  [[40,40,15],[90,20,0], extRoundedCube]
];

// The modules can be accessed directly
// from the array
for(i = [0:len(extrusionInfo)-1])
   translate(extrusionInfo[i][0])
      rotate(extrusionInfo[i][1]){
         height = 40;
         (extrusionInfo[i][2])(height);
      }