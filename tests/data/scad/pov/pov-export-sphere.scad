// A sphere straight from the primitive has non-planar quads, which POV-Ray ignores,
// so they must be exported as triangles. ($fn=5 is the smallest sphere with non-planar faces.)
sphere(10, $fn=5);
