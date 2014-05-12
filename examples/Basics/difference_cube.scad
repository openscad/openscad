echo(version=version());

module difference_cube()
{
	difference() {
		cube(30, center = true);
		sphere(20);
	}
}

difference_cube();

