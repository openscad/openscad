// Copyright 2011 Elmo Mäntynen
// LGPL 2.1

// Give a list of 4+4 points (check order) to form an 8 point polyhedron 
module connect_squares(points){
    polyhedron(points=points,
               triangles=[[0,1,2], [3,0,2], [7,6,5], [7,5,4], // Given polygons
                          [0,4,1], [4,5,1], [1,5,2], [2,5,6],  // Connecting
                          [2,6,3], [3,6,7], [3,4,0], [3,7,4]]);// sides
}
