// Issue #3118
// Verify recursion limit is reached when no arguments provided in call.
function sin(x) = sin(); 
echo(sin(30));