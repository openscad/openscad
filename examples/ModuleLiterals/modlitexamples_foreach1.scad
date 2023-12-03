
/*
Instantiate some extrusions in various poses without ModuleLiteral feature
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
   
// indices to identify each extrusion above
// in the poseInfo array
idxSquare = 0;
idxCircle = 1;
idxTriangle = 2;
idxOctagon = 3;
idxRoundedCube = 4; 
 
// array of transforms
// translations in col 0
// and rotations in col 1
// to match the above extrusion indices
poseInfo = [
  [[0,0,0],[0,0,0]], // Square
  [[10,10,0],[22.5,5,0]], // Circle
  [[20,20,5],[45,10,0]], // Triangle
  [[30,30,10],[67.5,15,0]], // Octagon
  [[40,40,15],[90,20,0]] //RoundedCube
];

// apply the poses to the extrusions 
for(i = [0:len(poseInfo)-1])
   translate(poseInfo[i][0])
      rotate(poseInfo[i][1]){
         height = 40;
         if (i == idxSquare)
           extSquare(height);
         else if (i == idxCircle)
           extCircle(height);
         else if (i == idxTriangle)
           extTriangle(height);
         else if (i == idxOctagon)
           extOctagon(height);
         else if (i == idxRoundedCube)
           extRoundedCube(height); 
      }
