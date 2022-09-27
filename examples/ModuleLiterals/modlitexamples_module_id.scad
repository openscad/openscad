
/*
 ModuleReference refrring to a module_id.
 
 In this style the module_literal is refering to an existing 
 module_id but no arguments are specified.
 The module reference can be though of in soam ways as  
 an alias name for the module
*/

my_cube = module cube;

/*
In this form the arguments are supplied when the module the 
module_reference is referring to is instantiated. The arguments 
have exactly the same form as for the original module.
*/

my_cube([10,2,30],center = false);