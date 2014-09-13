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
