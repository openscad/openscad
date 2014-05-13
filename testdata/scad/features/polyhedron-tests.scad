module polyhedrons() {
 polyhedron(points = [[1,0,0],[-1,0,0],[0,1,0],[0,-1,0],[0,0,1],[0,0,-1]],
            faces = [[0,4,2],[0,2,5],[0,3,4],[0,5,3],[1,2,4],[1,5,2],[1,4,3], [1,3,5]]);

 // One face flipped
 translate([2,0,0])
  polyhedron(points = [[1,0,0],[-1,0,0],[0,1,0],[0,-1,0],[0,0,1],[0,0,-1]],
            faces = [[0,4,2],[0,2,5],[0,3,4],[0,5,3],[1,2,4],[1,5,2],[1,3,4], [1,3,5]]);

 // All faces flipped
 translate([4,0,0])
  polyhedron(points = [[1,0,0],[-1,0,0],[0,1,0],[0,-1,0],[0,0,1],[0,0,-1]],
            faces = [[0,2,4],[0,5,2],[0,4,3],[0,3,5],[1,4,2],[1,2,5],[1,3,4], [1,5,3]]);

// Containing concave polygons
translate([6,0,0])
polyhedron(points=[
        [-0.8,-0.8,-0.8],
        [0,0,-0.8],
        [0.8,-0.8,-0.8],
        [0.8,0.8,-0.8],
        [-0.8,0.8,-0.8],
        [-0.8,-0.8,0.8],
        [0,0,0.8],
        [0.8,-0.8,0.8],
        [0.8,0.8,0.8],
        [-0.8,0.8,0.8],
    ],
    faces=[
        [0,1,2,3,4],
        [5,6,1,0],
        [6,7,2,1],
        [7,8,3,2],
        [8,9,4,3],
        [9,5,0,4],
        [9,8,7,6,5],
    ], convexity=2);
}

// Also concave polygon
translate([0,-4,0])
polyhedron(points=2*[[0,0,0],[2,0,0],[2,.2,0],[1,.2,0],[1,1,0],[0,1,0],[0,0,-1]],
           faces=[[5,4,3,2,1,0],[0,1,6],[1,2,6],[2,3,6],[3,4,6],[4,5,6],[5,0,6]],
           convexity=2);
polyhedrons();
translate([0,2,0]) difference() {
  polyhedrons();
  translate([3,0,2]) cube([8,3,3], center=true);
}

// dont crash (issue #703)
polyhedron(points = undef, faces = [[1, 2, 3]]);
polyhedron(points = [[0,0,0],[1,1,1]], faces = undef);
