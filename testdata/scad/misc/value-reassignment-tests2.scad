// Test reassignment where another variable has used the previous
// value before the reassignment. This could get messed up if order of
// assignment evaluation changes

myval = 2;
i = myval;
myval = 3;
echo(myval, i); // Should output 3, 2

