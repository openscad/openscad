// This causes OpenSCAD to crash. See source code comments:
// PolySetCGALEvaluator::evaluatePolySet(const ProjectionNode &node)
// Se also https://github.com/openscad/openscad/issues/80

projection(cut=true) translate([0,0,-4.999999]) cube(10, center=true);
