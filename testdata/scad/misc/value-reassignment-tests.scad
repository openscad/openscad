// Test reassignment which depends on a previously assigned variable,
// as this could be messed up if order of assignment evaluation
// changes

myval = 2;
i = 2;
myval = i * 2;
echo(myval, i); // Should output 4, 2

