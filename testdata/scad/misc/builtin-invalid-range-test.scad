inf = 1/0;
nan = inf / inf;
for(i=[1,0,-1,-inf,nan,inf]){
    echo(str("i=",i));
    cube([1,i,1]);
    sphere(i);
    cylinder(h=i);
    cylinder(r=i);
    square(i);
    square([1,i]);
    square([i,1]);
    circle(i);
}