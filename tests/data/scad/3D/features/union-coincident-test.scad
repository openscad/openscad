// Causes a CGAL assertion in CGALEvaluator::process()
e=0.000;
for (m = [ [ [ 0, 1, 0], [ 0, 0, 1], [ 1, 0, 0] ],
       [ [-1, 0, e], [ 0,-1, 0], [ 0, 0,-1] ] ] )
    multmatrix (m) cube([1,5,1], center=true);
