
   module subtract(a,b){
     difference(){
       a();
       b();
     }
   };

   subtract(
      module { 
         translate([-20,0,-20])
            cube([40,40,40]);
      }, 
      module sphere(d=20)
   );

