
module child1() {
	echo("child1");
}
module child2() {
	echo("child2");
}
module child3() {
	echo("child3");
}
module child4() {
	echo("child4");
}
module child5() {
	echo("child5");
}

module test_children_empty() {
	echo("Children empty: begin");
	children();
	echo("Children empty: end");
}
test_children_empty() {
	child1();child2();child3();child4();child5();
}

module test_children_scalar() {
	echo("Children scalar: begin");
	children(0); // child1
	children(4); // child5
	children(2); // child3
	children(5); // out
	children(-1); // out
	echo("Children scalar: end");
}
test_children_scalar() {
	child1();child2();child3();child4();child5();
}

module test_children_vector() {
	echo("Children vector: begin");
	children([4]); // child5 last
	children([0,3,1]); // child1, child4, child2
	children([5,-1]); // out, out
	echo("Children vector: end");
}
test_children_vector() {
	child1();child2();child3();child4();child5();
}

module test_children_range() {
	echo("Children range: begin");
	children([0:4]); // all
	children([1:2]); // child2, child3
	children([0:2:4]); // child1, child3, child5
	children([0:-1:4]); // out, out
	echo("Children range: end");
}
test_children_range() {
	child1();child2();child3();child4();child5();
}

// to avoid no object error
cube(1.0);
