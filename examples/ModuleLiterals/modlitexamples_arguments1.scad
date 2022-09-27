
/*
 ModuleLiterals can also be defined to take parameters.
 Here we define the module_alias to accept a height parameter
*/
modRef = module(height) cylinder(d = 5,h = height, center=true, $fn = 20);

/*
 The module_reference can be used to instantiate 
 the module it references, using the same syntax as
 is used for modules, including the syntax for supplying arguments.
*/
modRef(5);
