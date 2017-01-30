// License: LGPL 2.1

rodsize = 6;	//threaded/smooth rod diameter in mm
xaxis = 182.5;	//width of base in mm
yaxis = 266.5;	//length of base in mm


screwsize = 3;	//bearing bore/screw diameter in mm
bearingsize = 10;	//outer diameter of bearings in mm
bearingwidth = 4;	//width of bearings in mm


rodpitch = rodsize / 6;
rodnutsize = 0.8 * rodsize;
rodnutdiameter = 1.9 * rodsize;
rodwashersize = 0.2 * rodsize;
rodwasherdiameter = 2 * rodsize;
screwpitch = screwsize / 6;
nutsize = 0.8 * screwsize;
nutdiameter = 1.9 * screwsize;
washersize = 0.2 * screwsize;
washerdiameter = 2 * screwsize;
partthick = 2 * rodsize;
vertexrodspace = 2 * rodsize;


c = [0.3, 0.3, 0.3];
rodendoffset = rodnutsize + rodwashersize * 2 + partthick / 2;
vertexoffset = vertexrodspace + rodendoffset;


renderrodthreads = false;
renderscrewthreads = false;
fn = 36;



module rod(length, threaded) if (threaded && renderrodthreads) {
	linear_extrude(height = length, center = true, convexity = 10, twist = -360 * length / rodpitch, $fn = fn)
		translate([rodsize * 0.1 / 2, 0, 0])
			circle(r = rodsize * 0.9 / 2, $fn = fn);
} else cylinder(h = length, r = rodsize / 2, center = true, $fn = fn);


module screw(length, nutpos, washer, bearingpos = -1) union(){
	translate([0, 0, -length / 2]) if (renderscrewthreads) {
		linear_extrude(height = length, center = true, convexity = 10, twist = -360 * length / screwpitch, $fn = fn)
			translate([screwsize * 0.1 / 2, 0, 0])
				circle(r = screwsize * 0.9 / 2, $fn = fn);
	} else cylinder(h = length, r = screwsize / 2, center = true, $fn = fn);
	render() difference() {
		translate([0, 0, screwsize / 2]) cylinder(h = screwsize, r = screwsize, center = true, $fn = fn);
		translate([0, 0, screwsize]) cylinder(h = screwsize, r = screwsize / 2, center = true, $fn = 6);
	}
	if (washer > 0 && nutpos > 0) {
		washer(nutpos);
		nut(nutpos + washersize);
	} else if (nutpos > 0) nut(nutpos);
	if (bearingpos >= 0) bearing(bearingpos);
}


module bearing(position) render() translate([0, 0, -position - bearingwidth / 2]) union() {
	difference() {
		cylinder(h = bearingwidth, r = bearingsize / 2, center = true, $fn = fn);
		cylinder(h = bearingwidth * 2, r = bearingsize / 2 - 1, center = true, $fn = fn);
	}
	difference() {
		cylinder(h = bearingwidth - 0.5, r = bearingsize / 2 - 0.5, center = true, $fn = fn);
		cylinder(h = bearingwidth * 2, r = screwsize / 2 + 0.5, center = true, $fn = fn);
	}
	difference() {
		cylinder(h = bearingwidth, r = screwsize / 2 + 1, center = true, $fn = fn);
		cylinder(h = bearingwidth + 0.1, r = screwsize / 2, center = true, $fn = fn);
	}
}


module nut(position, washer) render() translate([0, 0, -position - nutsize / 2]) {
	intersection() {
		scale([1, 1, 0.5]) sphere(r = 1.05 * screwsize, center = true);
		difference() {
			cylinder (h = nutsize, r = nutdiameter / 2, center = true, $fn = 6);
			cylinder(r = screwsize / 2, h = nutsize + 0.1, center = true, $fn = fn);
		}
	}
	if (washer > 0) washer(0);
}


module washer(position) render() translate ([0, 0, -position - washersize / 2]) difference() {
	cylinder(r = washerdiameter / 2, h = washersize, center = true, $fn = fn);
	cylinder(r = screwsize / 2, h = washersize + 0.1, center = true, $fn = fn);
}

module rodnut(position, washer) render() translate([0, 0, position]) {
	intersection() {
		scale([1, 1, 0.5]) sphere(r = 1.05 * rodsize, center = true);
		difference() {
			cylinder (h = rodnutsize, r = rodnutdiameter / 2, center = true, $fn = 6);
			rod(rodnutsize + 0.1);
		}
	}
	if (washer == 1 || washer == 4) rodwasher(((position > 0) ? -1 : 1) * (rodnutsize + rodwashersize) / 2);
	if (washer == 2 || washer == 4) rodwasher(((position > 0) ? 1 : -1) * (rodnutsize + rodwashersize) / 2);
}


module rodwasher(position) render() translate ([0, 0, position]) difference() {
	cylinder(r = rodwasherdiameter / 2, h = rodwashersize, center = true, $fn = fn);
	rod(rodwashersize + 0.1);
}


rod(20);
translate([rodsize * 2.5, 0, 0]) rod(20, true);
translate([rodsize * 5, 0, 0]) screw(10, true);
translate([rodsize * 7.5, 0, 0]) bearing();
translate([rodsize * 10, 0, 0]) rodnut();
translate([rodsize * 12.5, 0, 0]) rodwasher();
translate([rodsize * 15, 0, 0]) nut();
translate([rodsize * 17.5, 0, 0]) washer();