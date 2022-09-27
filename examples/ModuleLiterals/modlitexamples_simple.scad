
/*
ModuleLiteral feature
 Simple alias syntax example
*/
// existing syntax
module my_module1() cube([10,10,10],center = true); 
color("red"){
   translate([-10,0,0]) my_module1();
}

// alternative ModuleLiteral syntax
my_module2 = module cube([10,10,10],center = true); 
color("blue"){
   translate([10,0,0]) my_module2();
}
