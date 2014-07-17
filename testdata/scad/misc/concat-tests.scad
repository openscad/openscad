u = undef;

echo("--- empty");
echo(concat());
echo(concat([]));
echo(concat([], []));
echo(concat([], [], []));

echo("--- single elements");
echo(concat(u));
echo(concat(true));
echo(concat(3));
echo(concat("abc"));
echo(concat([0:1:10]));

echo("--- single vectors");
echo(concat([1, 2, 3]));
echo(concat([[1, 2, 3]]));
echo(concat([[[1, 2, 3]]]));
echo(concat([[[1, 2, [3, 4], 5]]]));

echo("--- multiple elements");
echo(concat(3, 3));
echo(concat(1, 2, 3));
echo(concat(1, 2, 3, 4, 5));
echo(concat(1, "text", false, [1:0.5:3]));

echo("--- vector / element");
echo(concat([3, 4], u));
echo(concat([3, 4, 5], 6));
echo(concat([3, 4, 5, 6], true));
echo(concat([3, 4, "5", 6], "test"));
echo(concat([3, 4, true, 6], [4:1:3]));

echo("--- element / vector");
echo(concat(3, []));
echo(concat(3, [3, 4]));
echo(concat(true, [3, [4]]));
echo(concat("9", [1, 2, 3]));
echo(concat([6:2:9], [3, [4]]));

echo("--- vector / vector");
echo(concat([], [3, 4]));
echo(concat([[]], [3, 4]));
echo(concat([[2, 4]], [3, 4]));
echo(concat([5, 6], ["d", [3, 4]]));
echo(concat([[1, 0, 0], [2, 0, 0]], [3, 0, 0]));
echo(concat([[1, 0, 0], [2, 0, 0]], [[3, 0, 0]]));
echo(concat([[1, 0, 0], [2, 0, 0], [3, 0, 0]], [[4, 4, 4], [5, 5, 5]]));

echo("--- recursive function");
function r(i) = i > 0 ? concat(r(i - 1), [[i, i * i]]) : [];
echo(r(10));
