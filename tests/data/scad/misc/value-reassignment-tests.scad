// Test reassignment which depends on a previously assigned variable,
// as this could be messed up if order of assignment evaluation
// changes

myval = 2;
i = 2;
myval = i * 2; // This is not (yet) allowed as it will be evaluates in place of the first assignment
echo(myval, i); // Should output undef, 2

