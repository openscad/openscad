// Test for module with NaN arg
module infiniteLoop() infiniteLoop();
for (i=[0:sqrt(-1)]) infiniteLoop();

// Test list comprehension for with NaN arg
function infiniteFunc() = infiniteFunc();
a = [for (i=[0:sqrt(-1)]) infiniteFunc()];

echo("OK");
