for (x = [0:0.5:1], y = [0:0.5:1], z = [0:0.5:1])
	translate(40 * [x + 4 * z, y, 0])
		color([x, y, z])
			cube(10);
