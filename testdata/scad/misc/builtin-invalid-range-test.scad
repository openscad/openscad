inf = 1/0;
nan = inf / inf;
tests= [1,0,-1,-inf,nan,inf];

for(i=tests){
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

for(i=tests){
    scale(i)
    cube();
}
echo("Those are okay");
cylinder(r1=1,r2=0,h=10);
cylinder(r1=0,r2=1,h=10);