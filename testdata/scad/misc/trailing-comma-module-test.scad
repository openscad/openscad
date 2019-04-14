
//This basically let-module-test with trailing comma
//They are expected to work the same way as without the let-module-test which does not contain trailing comma 

a = 1;
b = 2;
c = 3;
echo(a, b, c,);

let (a = 5, b = a + 8, c = a + b,) {
	echo(a, b, c,);
	difference() {
		cube([a, b, c], center = true,);
		let (b = 2 * a, c = b * 2,) {
			echo(a, b, c,);
			rotate([0, 90, 0])
				cylinder(d = b, h = c, center = true,);
		}
	}
}