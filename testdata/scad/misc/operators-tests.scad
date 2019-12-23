nan = 0/0;
inf = 1/0;

for (lhs = [false,true,-1,0,1,nan,inf,"alpha","beta",[1,2,3],[1,2,4],[0:3],[0:1],undef],
     rhs = [false,true,-1,0,1,nan,inf,"alpha",[1,2,3],[0:3],undef]) {
     echo(lhs," == ",rhs,"->",lhs == rhs);
     echo(lhs," >  ",rhs,"->",lhs >  rhs);
     echo(lhs," >= ",rhs,"->",lhs >= rhs);
     echo(lhs," <  ",rhs,"->",lhs <  rhs);
     echo(lhs," <= ",rhs,"->",lhs <= rhs);
     echo(lhs," != ",rhs,"->",lhs != rhs);
}


// Check that vectors and their "equivalent" ranges compare in the same manner
// Note: ranges and vectors are still not comparable directly to each other
vec_vs_range = [
  [ [], [ 1:-1: 3] ], // empty ranges should compare equally
  [ [], [-1:-1: 1] ], // regardless of difference in "parameters"
  [ [ 1, 0,-1], [ 1:-1:-1] ],
  [ [ 1, 2, 3], [ 1: 1: 3] ],
  [ [ 1, 0,-1,-2], [ 1:-1:-2] ],
  [ [ 1, 2, 3, 4], [ 1: 1: 4] ],
];

for (lhs = vec_vs_range, rhs = vec_vs_range) {
  // Results should always be true
  echo(str("(",lhs[0]," == ",rhs[0],") == (",lhs[1]," == ",rhs[1],") -> "),
            (  lhs[0]   ==   rhs[0]  ) == (  lhs[1]   ==   rhs[1]  ));
  echo(str("(",lhs[0]," >  ",rhs[0],") == (",lhs[1]," >  ",rhs[1],") -> "),
            (  lhs[0]   >    rhs[0]  ) == (  lhs[1]   >    rhs[1]  ));
  echo(str("(",lhs[0]," >= ",rhs[0],") == (",lhs[1]," >= ",rhs[1],") -> "),
            (  lhs[0]   >=   rhs[0]  ) == (  lhs[1]   >=   rhs[1]  ));
  echo(str("(",lhs[0]," <  ",rhs[0],") == (",lhs[1]," <  ",rhs[1],") -> "),
            (  lhs[0]   <    rhs[0]  ) == (  lhs[1]   <    rhs[1]  ));
  echo(str("(",lhs[0]," <= ",rhs[0],") == (",lhs[1]," <= ",rhs[1],") -> "),
            (  lhs[0]   <=   rhs[0]  ) == (  lhs[1]   <=   rhs[1]  ));
  echo(str("(",lhs[0]," != ",rhs[0],") == (",lhs[1]," != ",rhs[1],") -> "),
            (  lhs[0]   !=   rhs[0]  ) == (  lhs[1]   !=   rhs[1]  ));
}
