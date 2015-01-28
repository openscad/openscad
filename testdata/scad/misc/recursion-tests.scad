function crash() = crash();
// Recursion as module parameter
echo(crash());
// Recursion as assignment
a = crash();

module crash() crash();
crash();
