
use <MCAD/regular_shapes.scad>
use <MCAD/boxes.scad>

shapeInfo = [
   [/*position = */ [0,0,0],/* rotation = */[0,0,0],/* shape = */ module{ 
      translate([-3.5,-3.5]) square([7,7]);
   }],
   /* circle = */ [[10,10],[0,0,5], module circle(d = 7, $fn = 30)],
   /*triangle = */ [[20,20],[0,0,10], module circle(d = 8, $fn = 3)],
   /* octagon = */ [[30,30],[0,0,15], module circle(d = 7, $fn = 8)],
   /* roundsquare = */ [[40,40],[0,0,20], module{
       hull(){
         for ( x = [-1:2:1], y = [-1:2:1]){
           offset = 3;
           translate([x*offset,y*offset]) circle(d=3,$fn=30);
         }
      }
   }]
];

for(elem = shapeInfo)
   translate(elem[0])
      rotate(elem[1]){
         m = elem[2];
         m();
      }