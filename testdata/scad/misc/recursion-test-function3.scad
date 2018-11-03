//Example without tail-recursion
//https://en.wikibooks.org/w/index.php?title=OpenSCAD_User_Manual/User-Defined_Functions_and_Modules&oldid=3379916#Recursive_functions
function add_up_to_tail(n, sum=0) =
    n==0 ?
        sum :
        add_up_to_tail(n-1, sum+n);

function add_up_to(n) = (
    n==0 ? 0 : n + add_up_to(n-1)
    );

//demonstration that tail recursion has a higher limit
a=10000;
echo(sum=add_up_to_tail(a));
echo(sum=add_up_to(a));

assert(false && "Did you intentionally change the recursion limit?");
