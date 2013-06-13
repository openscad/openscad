// Test reassignment where another variable has used the previous
// value before the reassignment. This could get messed up if order of
// assignment evaluation changes

myval = 2;
i = myval;
myval = 3;
echo(myval, i); // Should output 3, 3

// NB! This feels wrong, but it's a simulation of what happens
// when overriding a variable on the cmd-line: openscad -Dmyval=3 myfile.scad
// Since the intention is to override a top-level variable, the evaluation of the
// new expression must be done in the same place as the old.
// This is currently solved by appending the text given to the -D parameter to the end
// of the main file.

