//
// probe()
//
// Analyse the first children and compute geometric information.
// This is information is put in variables which are available during
// the evaluation of the subsequente children.
//
// The variables are:
// empty : true if the geometry if empty
// bbsize : bounding box size [x,y,z]
// bbcenter : center position of bounding box [x,y,z]
// bbmin : position of lowest corner of bounding box [x,y,z]
// bbmax : position of highest corner of bounding box [x,y,z]
//

// We measure the bounding box of someGeometry()

module someGeometry() { translate([5,10,15]) sphere(20,$fn=64); }

// this example will only print information on the geometry

probe() {
    someGeometry();
	echo("empty: ",empty);
	echo("bbox: ",bbmin," to ",bbmax);
	echo("size: ",bbsize," center: ",bbcenter);
}

// this example will also render the geometry
// with a bounding box drawn around it

probe() {
    someGeometry();
	echo("empty: ",empty);
	echo("bbox: ",bbmin," to ",bbmax);
	echo("size: ",bbsize," center: ",bbcenter);
    someGeometry();
    color("purple",0.3) translate(bbcenter) cube(bbsize,center=true);
}

