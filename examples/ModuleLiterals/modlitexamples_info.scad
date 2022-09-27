
/*
Showing a dictionary using ModuleLiterals as keys
Output on console only
*/
use <modlitexamples_find.scad>

mycube1 = module cube([20,20,40]);
mycube2 = module cube([30,20,20]);
mycylinder = module cylinder(d = 20, h = 30);

my_slot = module(dia,l){ 
   cen = (l-dia)/2;
   module c (x) {translate([x,0,0]){ circle(d= dia, $fn = 20);}};
   hull(){ c(-cen); c(cen);}
};

info_dict = [
  [mycube1, "The basic cube"],
  [mycube2, "Another fine cube"],
  [mycylinder, "A humble cylinder"],
  [my_slot, "A slot"]
];

module info (m) echo( "info = ", find(info_dict,m),m);

for ( m = [mycube1,my_slot,mycube2,mycylinder]){
   info(m);
}
