
//
// test volume information
//

module txt(t,sz=16) { color("blue") rotate([30,0,0])  text(t,sz,halign="center",valign="center"); }


//
// probe the children and print volumetric information
//

module info() {
  probe(volume=true,$exact=true) {
	children(); // the geometry to probe
    echo("empty",empty);
	echo("bbsize",bbsize);
	echo("bbcenter",bbcenter);
    echo("bbmin",bbmin);
	echo("bbmax",bbmax);

	echo("volume",volume);
	echo("centroid",centroid);
    children();
    translate([0,bbmin[1]-20,0]) txt(str(round(volume/1000*10)/10,"ml"),8);
  }
}

module move(d) { translate([d*30,0,0]) children(); }


move(0) info() union() { sphere(r=10); cube(20); }
move(1) txt("=");
move(2) info() intersection() { sphere(r=10); cube(20); }
move(3) txt("+");
move(4) info() difference() { sphere(r=10); cube(20); }
move(5) txt("+");
move(6) info() difference() { cube(20);sphere(r=10); }

