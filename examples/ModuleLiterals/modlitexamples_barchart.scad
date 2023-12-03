use <MCAD/regular_shapes.scad>

/*
 showing ModuleLiterals as Module arguments
*/

module barchart ( bar, data, spacing){
    for ( i= [0:len(data)-1]){
        translate([spacing *  i, 0,0]){
           bar(data[i],spacing);
        }
    }
};

roundbar = module (height, spacing)
{
   cylinder( d= spacing -1, h = height);
};

squarebar = module (height, spacing)
{
   translate([-(spacing-1)/2,-(spacing-1)/2,0]){
      cube([spacing-1,spacing-1,height]);
   }
};

triangularbar = module (height, spacing){
   linear_extrude( height = height){
      triangle(spacing/2);
   }
};

color("blue"){
   barchart(
      roundbar,
      [100,150,70,50,70,30],
      10
   );
}

color("red"){
   translate( [0,50,0]){
      barchart(
         squarebar,
         [70,80,100,120,50,20],
         10
      );
   }
}

color("yellow"){
   translate( [0,100,0]){
      barchart(
         triangularbar,
         [30,50,100,120,80,20],
         10
      );
   }
}


