
/*
More complex example using ModuleLiterals
*/
// modified from
// https://en.m.wikibooks.org/wiki/OpenSCAD_User_Manual/User-Defined_Functions_and_Modules#Object_modules

use <MCAD/regular_shapes.scad>
use <MCAD/boxes.scad>
use <modlitexamples_find.scad>

module end_customisations(){}

shapeLookup = [
  ["square", module (height) { translate([-3.5,-3.5,0]) cube([7,7,height]);}],
  ["cylinder", module (height) cylinder(d = 7, h = height)],
  ["triangle", module (height) triangle_prism(height,4)],
  ["octagon", module (height) octagon_prism(height,4)],
  ["rounded cube", module(height) {
     translate([-3.5,-3.5,0]){
       roundedCube([7,7, height],2,false,false);
     }
  }],
  ["cone", module (height) { cylinder(d1 = 7, d2 = 0, h = height);}],
  ["ellipse", module (height) { 
     translate([0,0,height/2]){
       scale([1,1,height/7]){
         sphere(d = 7, $fn =30);
       }
    }
  }] 
];

translate([0,20,0])
   ShowColorBars(shapeLookup, Expense);
   
ColorBreak=[[0,""],
           [20,"lime"],  // upper limit of color range
           [40,"greenyellow"],
           [60,"yellow"],
           [75,"LightCoral"],
           [200,"red"]];
Expense=[16,20,25,85,52,63,45];
shapes = [
   "square", 
   "cylinder",
   "octagon",
   "ellipse",
   "rounded cube", 
   "triangle",
   "cone"
];
   
module ColorBar( shape, value,period,range){  // 1 color on 1 bar

   RangeHi = ColorBreak[range][0];
   RangeLo = ColorBreak[range-1][0];

   height = (value > RangeHi)
      ? RangeHi-RangeLo
      : (value > RangeLo)
         ? value-RangeLo
         : undef
   ;
   if (is_num(height) ){
      color( ColorBreak[range][1] ) 
         translate([10*period,0,RangeLo])
            shape(height);
   }  
}
  
module ShowColorBars(shape, values){
    for (month = [0:len(values)-1], range = [1:len(ColorBreak)-1])
      ColorBar(find(shape,shapes[month]), values[month],month,range);
}
