
/*
children with Module Literal
*/

x_rotate = module (x) { rotate([x,0,0]) children();};

r = module { x_rotate(45) children(); };

cone = module (height) { 
  cylinder( d1 = 0, d2 = 20, h = height,center = true);
};

ar = [
   module (a) { r() children();},
   module (a) { x_rotate(a) children();}
];

child_ar = [ 
   module (dia) cone(dia), 
   module (dia) cylinder( d = dia, h = 5, $fn = 20)
];

// inst 1
// module_reference with child module literal expression
translate([0,0,30]){
  r() (child_ar[0])(20);
}

// inst 3 using module_literal expression with child module
translate([0,0,70]){
   (ar[1])(-30) cone(100);
}

// inst 4 using module_literal expression with child module_literal expression
translate([0,0,100]){
   (ar[1])(-30) (child_ar[1])(100);
}
