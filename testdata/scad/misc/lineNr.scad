function fn () = $linenumber;
function fn2 (a) = concat(a, " " , $linenumber);

module test(){
        echo($linenumber);
}

echo($linenumber);
echo($linenumber);
echo(fn());
test();
echo(fn2($linenumber));

echo($mainfilename);