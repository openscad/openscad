nan = 0/0;

for (lhs=[0,nan],rhs=[0,nan]) {
	echo(lhs," == ",rhs,"->",lhs == rhs);
	echo(lhs," >  ",rhs,"->",lhs >  rhs);
	echo(lhs," >= ",rhs,"->",lhs >= rhs);
	echo(lhs," <  ",rhs,"->",lhs <  rhs);
	echo(lhs," <= ",rhs,"->",lhs <= rhs);
	echo(lhs," != ",rhs,"->",lhs != rhs);
}
