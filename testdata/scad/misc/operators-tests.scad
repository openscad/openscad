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
ranges = [
  [ 1:-1: 3], // empty ranges should compare equally
  [-1:-1: 1], // regardless of difference in "parameters"
  [ 1:-1:-1], 
  [ 1: 1: 3],
  [ 1:-1:-2],
  [ 1: 1: 4],
  [ 0: 1: 0]
];

for (lhs = ranges, rhs = ranges) {
  // expand ranges to vectors
  lhs_v = [for(x=lhs) x]; rhs_v = [for(x=rhs) x];

  eq_v = lhs_v == rhs_v;    eq_r = lhs == rhs;
  gt_v = lhs_v >  rhs_v;    gt_r = lhs >  rhs;
  ge_v = lhs_v >= rhs_v;    ge_r = lhs >= rhs;
  lt_v = lhs_v <  rhs_v;    lt_r = lhs <  rhs;
  le_v = lhs_v <= rhs_v;    le_r = lhs <= rhs;
  ne_v = lhs_v != rhs_v;    ne_r = lhs != rhs;
  // Boolean results between vector comparison and range comparison should always match   ||||||||||
  //                          In other words the rightmost value in echos is always true: VVVVVVVVVV
  echo(str("(",lhs_v," == ",rhs_v,") == (",lhs," == ",rhs,") -> ",eq_v," == ",eq_r," -> ",eq_v==eq_r));
  echo(str("(",lhs_v," >  ",rhs_v,") == (",lhs," >  ",rhs,") -> ",gt_v," == ",gt_r," -> ",gt_v==gt_r));
  echo(str("(",lhs_v," >= ",rhs_v,") == (",lhs," >  ",rhs,") -> ",ge_v," == ",ge_r," -> ",ge_v==ge_r));
  echo(str("(",lhs_v," <  ",rhs_v,") == (",lhs," <  ",rhs,") -> ",lt_v," == ",lt_r," -> ",lt_v==lt_r));
  echo(str("(",lhs_v," <= ",rhs_v,") == (",lhs," <= ",rhs,") -> ",le_v," == ",le_r," -> ",le_v==le_r));
  echo(str("(",lhs_v," != ",rhs_v,") == (",lhs," != ",rhs,") -> ",ne_v," == ",ne_r," -> ",ne_v==ne_r));
}
