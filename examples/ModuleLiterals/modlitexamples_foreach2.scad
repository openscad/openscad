
/*
Instantiate some extrusions in various poses using ModuleLiteral feature - Simplest form
*/
use <MCAD/regular_shapes.scad>
use <MCAD/boxes.scad>

// extrusion modules parameterised on height
module extSquare(height) 
  translate([-3.5,-3.5,0]) cube([7,7,height]);
module extCircle(height) 
  cylinder(d = 7, h = height, $fn = 30);
module extTriangle(height) triangle_prism(height,5);
module extOctagon(height) octagon_prism(height,4);
module extRoundedCube(height) 
   translate([-3.5,-3.5,0]) 
     roundedCube([7,7, height],2,false,false);

// array of transforms
// translations in col 0
// and rotations in col 1
// to match the above extrusion indices
poseInfo = [
  [[0,0,0],[0,0,0]],
  [[10,10,0],[22.5,5,0]],
  [[20,20,5],[45,10,0]],
  [[30,30,10],[67.5,15,0]],
  [[40,40,15],[90,20,0]]
];

/*
using ModuleLiterals feature, we can store Module Literals representing the modules in an array
*/
extrusions = [
   module extSquare,
   module extCircle,
   module extTriangle,
   module extOctagon,
   module extRoundedCube,
];

// The modules can be accessed directly
// from the array
for(i = [0:len(poseInfo)-1])
   translate(poseInfo[i][0])
      rotate(poseInfo[i][1]){
         height = 40;
         extrusion = extrusions[i];
         extrusion(height);
      }
