test = "test";

// Correct usage cases
for(i = [0:len(test)-1]) {
	echo(test[i]);
}

// Out of bounds
echo(test[-1]);
echo(test[len(test)]);

// Invalid index type
echo(test["test"]);
echo(test[true]);
echo(test[false]);
echo(test[[0]]);
echo(test[1.7]);