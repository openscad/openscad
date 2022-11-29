
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

// inst 1
r() cylinder( d = 20, h = 5, $fn = 20);

// inst 2
translate([0,0,30]){
  r() cone(20);
}

// inst 3
translate([0,0,70]){
   (ar[1])(-30) cone(100);
}
