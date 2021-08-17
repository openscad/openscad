u=undef;

echo(norm([]));
echo(norm([1]));
echo(norm([1,2]));
echo(norm([1,2,3]));
echo(norm([1,2,3,4]));
echo(norm());

echo(norm([1,2,0/0]));
echo(norm([1,2,1/0]));
echo(norm([1,2,-1/0]));

echo(norm(""));
echo(norm("abcd"));
echo(norm(true));
echo(norm([1:4]));

echo(norm([1, 2, "a"]));
echo(norm([1, 2, []]));
echo(norm([1, 2, [1]]));
echo(norm([1, 2, [1:3]]));
echo(norm([[1,2,3,4],[1,2,3],[1,2],[1]]));

echo(norm(u));
echo(norm(u, u));
echo(norm(a, a));
