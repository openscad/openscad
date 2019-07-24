// This crashes OpenSCAD including 2011.06 in PolyReducer due to two vertices of 
// a triangle evaluating to the same index
linear_extrude(height=2) 
  polygon(points=[[0, 0],
		  [1, 0],
		  [1.0014, 1],
		  [1, 1],
		  [0, 1]], 
           paths=[[0,1,2,3,4]]);
