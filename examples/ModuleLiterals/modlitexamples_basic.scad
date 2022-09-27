
/*
 ModuleLiterals - very simple example

 The variable modRef is called a module_reference.
 The expression on the right side of the equals sign
 is a module_literal
 
*/
modRef = module cube([10,10,20], center=true);

/*
 The module_reference can be used to instantiate 
 the module it references, using the same syntax as
 is used for modules
*/
modRef();
