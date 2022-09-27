
/*
children with Module Literal
*/

x_rotate = module (x) { rotate([x,0,0]) children();};

r = module { x_rotate(45) children(); };

r() cylinder( d = 20, h = 5, $fn = 20);


cone = module (height) { 
  cylinder( d1 = 0, d2 = 20, h = height,center = true);
};

translate([0,0,30]){
  r() cone(20);
}
