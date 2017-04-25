
//
// probe(volume=true)
//
// compute volume and center of mass of ths first child
// (and bounding box, which is always computed)
//
// Because the computation can be slow, you must specify volume=true.
//
// The variables which will be defined for the subsequent children are:
//
// volume : the volume of the first child
// centroid : position of the center of mass [x,y,z]
// 
//


// sample geometry
// each cube is 20x20x20 (volume 8000)
// with an overlap of 10x10x10 (volume 1000)
// we should get a total of 8000+8000-1000 = 15000 volume
// and centroid at [15,15,15]

module thing() {
    cube(20);
    // bug a 20,20,20... single convex part instead of 2...
    translate([10,10,10]) cube(20);
}

// display volume of two cubes, and put a sphere at the center of mass


probe(volume=true) {
    thing(); // the thing to measure
    // the result, with measurements available
    echo("volume=",volume," centroid=",centroid);
    %thing();
    color("red") translate(centroid) sphere(r=1,$fn=32);
}
