echo(cross([2, 3, 4], [5, 6, 7]));
echo(cross([2, 1, -3], [0, 4, 5]));
echo(cross([2, 1, -3], cross([2, 3, 4], [5, 6, 7])));

echo(cross([2, 0/0, -3], [0, 4, 5]));
echo(cross([2, 1/0, -3], [0, 4, 5]));
echo(cross([2, -1/0, -3], [0, 4, 5]));

echo(cross(0));
echo(cross(0, 1));
echo(cross(0, "a"));
echo(cross(0, 1, 3));
echo(cross([0], [1]));
echo(cross([1, 2, 3], [1, 2]));
echo(cross([1, 2, 3], [1, 2, "a"]));
echo(cross([1, 2, 3], [1, 2, [0]]));
echo(cross([1, 2, 3], [1, 2, [1:2]]));

// 2D cross
echo(cross([2, 3], [5, 6]));
