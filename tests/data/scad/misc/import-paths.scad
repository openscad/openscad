// Tests that the CSG dumped has a path relative to the main file.
// Note that this is different from the SCAD behavior.  The SCAD behavior is that import() is
// relative to the file that contains the import().  Because SCAD file boundaries are lost in CSG,
// the paths exported to CSG must all be relative to the CSG file, and we assume that's adjacent
// to the SCAD file.

// This main file imports the STL relative to itself, and that's what ends up in the CSG.
// The included file, however, imports that same STL relative to *itself*, as ../.../../stl/empty.stl,
// and ends up in the CSG relative to *this* file, as ../../stl/empty.stl.
import("../../stl/empty.stl");

include <sub1/import-included.scad>
