// A polygon with an edge on the axis: rotate_extrude() collapses those points to one vertex
// each, which gives triangles that use a vertex twice. Exporting without rendering must
// still work (lib3mf v1 rejected such triangles, #6716).
rotate_extrude($fn=4) polygon([[0, 0], [20, 20], [0, 20]]);
