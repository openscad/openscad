// A sphere straight from the primitive has non-planar quads, which POV-Ray ignores,
// so they must be exported as triangles.
sphere(10, $fn=8);
