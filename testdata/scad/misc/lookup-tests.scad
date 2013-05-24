echo(lookup(undef, undef));
echo(lookup(undef, [undef]));
echo(lookup(undef, [[undef]]));
echo(lookup(undef, [[undef, undef]]));
echo(lookup(0, [[0, 0]]));
echo(lookup(0.5, [[0, 0],
                  [1, 1]]));

table = [[-1,  -5],
         [-10, -55],
         [0,    0],
         [1,    3],
         [10,   333]];
indices = [-20,-10,-9.9, -0.5, 0, 0.3, 1.1, 10, 10.1];
for (i=[0:len(indices)-1]) {
  echo(lookup(indices[i], table));
}
