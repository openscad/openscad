/*
ModuleReference to anonymous module

In this form the module_reference refers to an anonymous module.
The module can take arguments or none. If there are no arguments 
the brackets are optional. Note that in contrast to module definitions
module literals are expressions and so in this context the statement must be termeinated with a semicolon!
*/
use <modlitexamples_airfoil.scad>

wing = module (span,chord, dihedral, taper =0,twist =0) {

    // the braces are optional in case there are no parameters
     wing_panel = module{  
    // wing_panel = module() { // alternative form
       rotate([90-dihedral,0,0])
          linear_extrude( height = span/2, scale = (1-taper), twist = -twist)
            scale([chord,chord])
              translate([-0.4,0,0])
                   airfoil();
    }; //##### <--- note the semicolon!

    wing_panel();
    mirror([0,1,0]){
      wing_panel();
    }

}; //##### <--- note the semicolon!

wing(1000,170,9,0.25,1);
