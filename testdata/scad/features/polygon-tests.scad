polygon();
polygon([]);
polygon([[],[]]);
polygon([[[]]]);
translate([2,0,0]) polygon([[0,0], [1,0], [1,1]]);
translate([0,2,0]) polygon([[0,0]]);
translate([2,2,0]) polygon([[0,0],[1,1]]);
translate([2,2,0]) polygon([[0,0],[1,1],[2,2]]);
translate([0,-2,0]) polygon(points=[[0,0], [1,0], [1,1], [0,1]]);
translate([0,-4,0]) polygon(points=[[0,0], [1,0], [1,1], [0,1]], paths=[]);
translate([2,-2,0]) polygon([[0,0], [1,0], [0.8,0.5], [1,1], [0,1]]);

points = [[0,0], [0.5,-0.2], [1,0], [1.2,0.5], [1,1], [0.5,1.2], [0,1], [-0.2,0.5]];
translate([-2,0,0]) polygon(points);
translate([-2,-2,0]) polygon(points=points, paths=[[0,1,2,3], [4,5,6,7]]);
translate([2,-4,0]) polygon([[0,0], [1,0], [1,1], [0,0]]);

// FIXME: convexity
