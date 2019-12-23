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
  [ [ 0 ], [ 0 : 1: 0] ]
];

for (lhs = vec_vs_range, rhs = vec_vs_range) {
  lh_v = lhs[0];     lh_r = lhs[1];
  rh_v = rhs[0];     rh_r = rhs[1];
  eq_v = lh_v==rh_v; eq_r = lh_r==rh_r;
  gt_v = lh_v> rh_v; gt_r = lh_r> rh_r;
  ge_v = lh_v>=rh_v; ge_r = lh_r>=rh_r;
  lt_v = lh_v< rh_v; lt_r = lh_r< rh_r;
  le_v = lh_v<=rh_v; le_r = lh_r<=rh_r;
  ne_v = lh_v!=rh_v; ne_r = lh_r!=rh_r;
  // Boolean results between vector comparison and range comparison should always match   ||||||||||
  //                          In other words the rightmost value in echos is always true: VVVVVVVVVV
  echo(str("(",lh_v," == ",rh_v,") == (",lh_r," == ",rh_r,") -> ",eq_v," == ",eq_r," -> ",eq_v==eq_r));
  echo(str("(",lh_v," >  ",rh_v,") == (",lh_r," >  ",rh_r,") -> ",gt_v," == ",gt_r," -> ",gt_v==gt_r));
  echo(str("(",lh_v," >= ",rh_v,") == (",lh_r," >  ",rh_r,") -> ",ge_v," == ",ge_r," -> ",ge_v==ge_r));
  echo(str("(",lh_v," <  ",rh_v,") == (",lh_r," <  ",rh_r,") -> ",lt_v," == ",lt_r," -> ",lt_v==lt_r));
  echo(str("(",lh_v," <= ",rh_v,") == (",lh_r," <= ",rh_r,") -> ",le_v," == ",le_r," -> ",le_v==le_r));
  echo(str("(",lh_v," != ",rh_v,") == (",lh_r," != ",rh_r,") -> ",ne_v," == ",ne_r," -> ",ne_v==ne_r));
}
