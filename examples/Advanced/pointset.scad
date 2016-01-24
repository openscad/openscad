// Example of CGAL pointset primitive

// Pointset source:
//  https://github.com/CGAL/cgal/blob/master/Point_set_processing_3/examples/Point_set_processing_3/data/oni.xyz


pointset_xyz=read_xyz("oni.xyz");
pointset(); // display usage
scale(40) pointset(pointset_xyz,verbose=true);

