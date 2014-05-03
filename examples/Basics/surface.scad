// surface.dat generated using octave:
//   d = (sin(1:0.2:10)' * cos(1:0.2:10)) * 10;
//   save("surface.dat", "d");

echo(version=version());

intersection()
{
	surface(file = "surface.dat",
		center = true, convexity = 5);
	
	rotate(45, [0, 0, 1])
	surface(file = "surface.dat",
		center = true, convexity = 5);
}
