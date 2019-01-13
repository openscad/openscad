inf = 1/0;
nan = inf / inf;
tests= [1,0,-1,-inf,nan,inf,"test"];
echo("Primitives");
for(i=tests){
    echo(str("i=",i,", primitive test"));

    cube([1,i,1]);
    sphere(i);
    cylinder(h=i);
    cylinder(r=i);
    square(i);
    square([1,i]);
    square([i,1]);
    circle(i);
    polyhedron(
      points=[ [10,10,0],[10,-10,0],[-10,-10,0],[-10,10,0], // the four points at base
               [0,0,i]  ],                                 // the apex point 
      faces=[ [0,1,4],[1,2,4],[2,3,4],[3,0,4],              // each triangle side
                  [1,0,3],[2,1,3] ]                         // two triangles for square base
     );
}
echo("Transformations");
for(i=tests){
    echo(str("i=",i,", transformation test"));
    scale(i)
    sphere();
    rotate(i);
    cube();
    translate([0,i,0])
    cylinder();
}
echo("Those are okay");
cylinder(r1=1,r2=0,h=10);
cylinder(r1=0,r2=1,h=10);
