
module example003()
{
	difference() {
		union() {
			cube([30, 30, 30], center = true);
			cube([40, 15, 15], center = true);
			cube([15, 40, 15], center = true);
			cube([15, 15, 40], center = true);
		}
		union() {
			cube([50, 10, 10], center = true);
			cube([10, 50, 10], center = true);
			cube([10, 10, 50], center = true);
		}
	}
}

example003();

