//Example without tail-recursion
//https://en.wikibooks.org/w/index.php?title=OpenSCAD_User_Manual/User-Defined_Functions_and_Modules&oldid=3379916#Recursive_functions

function add_up_to(n) = (
    n==0 ? 0 : n + add_up_to(n-1)
    );

echo(sum10=add_up_to(10));
echo(sum=add_up_to(-1));


