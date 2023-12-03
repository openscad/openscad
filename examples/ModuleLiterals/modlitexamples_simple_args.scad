
/*
ModuleLiteral feature
 Simple alias syntax example
 demonstrating arguments to the ModuleLiteral
*/
xposition = 20;

// existing syntax
module my_module1(xpos) {
  translate([-xpos,0,0])
     cube([xpos,10,10],center = true); 
}

// alternative ModuleLiteral syntax
my_module2 = module(xpos) {
  translate([xpos,0,0])
    cube([xpos,10,10],center = true);
};  // NOTE trailing semicolon
  
color("red"){
   my_module1(xposition);
}

color("blue"){
  my_module2(xposition);
}
