
   module subtract(a,b){
     difference(){
       translate( a.pos){
        a();
       }
       translate( (b.pos == undef)? [0,0,-5]:b.pos)
       b();
     }
   };

   subtract(
      module { 
         pos = [-20,0,-20];
         cube([40,40,40]);
      }, 
      module sphere(d=20)
   );

