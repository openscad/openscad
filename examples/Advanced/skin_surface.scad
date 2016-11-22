// Example of CGAL skin_surface primitive

skin_surface(); // Display usage
scale(20) translate([-0.5,-0.5,-0.5]) {
    nrand=pow(5,3);
    rand_nums=rands(0,1,3*nrand,0);
    pts_random = [ for(i=[0:3:3*(nrand-1)]) [ rand_nums[i],rand_nums[i+1],rand_nums[i+2] ] ];
    pts_corners = [ for(x=[0,1],y=[0,1],z=[0,1]) [x,y,z] ];
    skin_surface(points=concat(pts_random,pts_corners),weight=(0.5+4*$t*(1-$t))/nrand,subdivisions=0,grow_balls=true,verbose=true);
}

