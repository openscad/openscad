use <MCAD/regular_shapes.scad>

/*
 Suppose we want the option of a 
   square , cylindrical, triangular or octagonal box, 
   depending on a choice value
*/

//choose shape idx 0 - 3.
choice = 1;

module end_customisations(){}

chooseSquare = 0;
chooseCylindrical = 1;
chooseTriangular = 2;
chooseOctoganal = 3;

assert( choice >=0 && choice < 4);

//In traditional  openscad we might do
module my_box1(choice) 
   if(choice == chooseSquare)
      translate([-5,-5,0]) cube([10,10,10]);
   else if (choice == chooseCylindrical)
      cylinder(d = 10, h = 10);
   else if (choice == chooseTriangular)
      triangle_prism(10,5);
   else if (choice == chooseOctoganal)
      octagon_prism(10,5);
 
module shape1() my_box1(choice); 

color("red"){
  translate([-10,0,0]) shape1();
}
// Using ModuleLiteral syntax we could do
choices = [
  /* square =  */ module { translate([-5,-5,0]) cube([10,10,10]);},
  /* cylinder = */  module cylinder(d = 10, h = 10),
  /* triangle = */ module triangle_prism(10,5),
  /* octagon = */ module octagon_prism(10,5)
];
shape2 = choices[choice];
color("blue"){
  translate([10,0,0]) shape2();
}









