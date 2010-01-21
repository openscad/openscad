
module screw(type = 2, r1 = 15, r2 = 20, n = 7, h = 100, t = 8)
{
	linear_extrude(height = h, twist = 360*t/n, convexity = t)
	difference() {
		circle(r2);
		for (i = [0:n-1]) {
				if (type == 1) rotate(i*360/n) polygon([
						[ 2*r2, 0 ],
						[ r2, 0 ],
						[ r1*cos(180/n), r1*sin(180/n) ],
						[ r2*cos(360/n), r2*sin(360/n) ],
						[ 2*r2*cos(360/n), 2*r2*sin(360/n) ],
				]);
				if (type == 2) rotate(i*360/n) polygon([
						[ 2*r2, 0 ],
						[ r2, 0 ],
						[ r1*cos(90/n), r1*sin(90/n) ],
						[ r1*cos(180/n), r1*sin(180/n) ],
						[ r2*cos(270/n), r2*sin(270/n) ],
						[ 2*r2*cos(270/n), 2*r2*sin(270/n) ],
				]);
		}
	}
}

module nut(type = 2, r1 = 16, r2 = 21, r3 = 30, s = 6, n = 7, h = 100/5, t = 8/5)
{
	difference() {
		cylinder($fn = s, r = r3, h = h);
		translate([ 0, 0, -h/2 ]) screw(type, r1, r2, n, h*2, t*2);
	}
}

translate([ -30, 0, 0 ])
screw();

translate([ 30, 0, 0 ])
nut();
