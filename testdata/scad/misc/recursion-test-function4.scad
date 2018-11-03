//recursion detected calling an in built function
function recurse(x) = max(x, recurse(x));

x = recurse(1);