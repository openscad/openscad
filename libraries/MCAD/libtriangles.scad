//Copyright (C) 2013 Alex Davies
//License: LGPL 2.1 or later
//todo, make library work with negative lengths by adding triangles to the inside of every surface. basicaly copy and paste the current triangles set and reverse the first and last digit of every triangle. In 4 character traingles switcht the middle ones around as well. Not sure if that' actually useful though.

module rightpyramid(rightpyramidx, rightpyramidy, rightpyramidz) {
	polyhedron ( points = [[0,0,0], 
			[rightpyramidx, 0, 0], 
			[0, rightpyramidy, 0], 
			[rightpyramidx, rightpyramidy, 0],
			[rightpyramidx/2, rightpyramidy, rightpyramidz]], 

	triangles = [[0,1,2],[2,1,3],[4,1,0],[3,1,4],[2,3,4],[0,2,4]]);

}

module cornerpyramid(cornerpyramidx, cornerpyramidy, cornerpyramidz) {
	polyhedron ( points = [[0,0,0], 
			[cornerpyramidx, 0, 0], 
			[0, cornerpyramidy, 0], 
			[cornerpyramidx, cornerpyramidy, 0],
			[0, cornerpyramidy, cornerpyramidz]], 

	triangles = [[0,1,2],[2,1,3],[4,1,0],[3,1,4],[2,3,4],[0,2,4]]);

}

module eqlpyramid(eqlpyramidx, eqlpyramidy, eqlpyramidz) {
	polyhedron ( points = [[0,0,0], 
			[eqlpyramidx, 0, 0], 
			[0, eqlpyramidy, 0], 
			[eqlpyramidx, eqlpyramidy, 0],
			[eqlpyramidx/2, eqlpyramidy/2, eqlpyramidz]], 

	triangles = [[0,1,2],[2,1,3],[4,1,0],[3,1,4],[2,3,4],[0,2,4]]);

}


module rightprism(rightprismx,rightprismy,rightprismz){
	polyhedron ( points = [[0,0,0],
			[rightprismx,0,0],
			[rightprismx,rightprismy,0],
			[0,rightprismy,0],
			[0,rightprismy,rightprismz],
			[0,0,rightprismz]], 
	triangles = [[0,1,2,3],[5,1,0],[5,4,2,1],[4,3,2],[0,3,4,5]]);
}



module eqlprism(rightprismx,rightprismy,rightprismz){
	polyhedron ( points = [[0,0,0],
			[rightprismx,0,0],
			[rightprismx,rightprismy,0],
			[0,rightprismy,0],
			[rightprismx/2,rightprismy,rightprismz],
			[rightprismx/2,0,rightprismz]], 
	triangles = [[0,1,2,3],[5,1,0],[5,4,2,1],[4,3,2],[0,3,4,5]]);
}

