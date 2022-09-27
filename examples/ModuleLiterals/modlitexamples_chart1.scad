
// modified from
// https://en.m.wikibooks.org/wiki/OpenSCAD_User_Manual/User-Defined_Functions_and_Modules#Object_modules
// HINT: Use animate with Steps = 4 and FPS = 5
use <MCAD/regular_shapes.scad>
//choose shape idx 0 - 3.
choice = ($t) * 4;

module end_customisations(){}
// shape indices
chooseSquare = 0;
chooseCylindrical = 1;
chooseTriangular = 2;
chooseOctoganal = 3;

assert( choice >=0 && choice < 4);
choices = [
  module (height) { translate([-3.5,-3.5,0]) cube([7,7,height]);},
  module (height) cylinder(d = 7, h = height),
  module (height) triangle_prism(height,4),
  module (height) octagon_prism(height,3)
];

translate([0,0,0])
   ShowColorBars(choices[choice], Expense);
   
ColorBreak=[[0,""],
           [20,"lime"],  // upper limit of color range
           [40,"greenyellow"],
           [60,"yellow"],
           [75,"LightCoral"],
           [200,"red"]];
           
Expense=[16,20,25,85,52,63,45];
   
module ColorBar( shape, value,period,range){  // 1 color on 1 bar
   RangeHi = ColorBreak[range][0];
   RangeLo = ColorBreak[range-1][0];
   color( ColorBreak[range][1] ) 
      
   translate([10*period,0,RangeLo])
      if (value > RangeHi)      shape(RangeHi-RangeLo);
      else if (value > RangeLo) shape(value-RangeLo);
  }  
  
module ShowColorBars(shape, values){
    for (month = [0:len(values)-1], range = [1:len(ColorBreak)-1])
      ColorBar(shape, values[month],month,range);
}
