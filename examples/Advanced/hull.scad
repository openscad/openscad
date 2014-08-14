echo(version=version());

module crystal() {

 r_corner=3;
 fn_base=10;
 r1=25-r_corner;
 h1=25-r_corner;
 h2=40;
 fn_sphere=fn_base*3;

 hull(){

  translate([0,0,h1])sphere(r_corner,$fn=fn_sphere);
  translate([0,0,-h2-h1])sphere(r_corner,$fn=fn_sphere);

  for(i=[0:fn_base-1]){

   rotate(360/fn_base*i)translate([r1,0,0])sphere(r_corner,$fn=fn_sphere);
   rotate(360/fn_base*i)translate([r1,r1,-h1])sphere(r_corner,$fn=fn_sphere);
   rotate(360/fn_base*i)translate([r1,0,-h2])sphere(r_corner,$fn=fn_sphere);

   }

  }

 }

crystal();
