
// display a text in front of an object
module info() {
    probe(volume=true) {
        children();
        color("blue") translate([0,bbmin[1]-20,0])  rotate([30,0,0])  text(str(round(volume/1000*10)/10,"ml"),halign="center",valign="center");
    }
}


module inside(n=50) {
    translate([0,0,20])
    hull() {
        sphere(r=20);
        translate([0,0,n]) cylinder(r=10,h=10);
    }
}

module inside2(n=50) {
    translate([0,0,n])
    hull() {
        sphere(r=n);
        translate([0,0,50-n+20]) cylinder(r=10,h=10);
    }
}

module inside3(n=50) {
    translate([0,0,20])
    hull() {
        sphere(r=20);
        translate([0,0,n]) cylinder(r=n,h=10);
    }
}


module wall() { translate([0,0,-3.02]) cylinder(r=3,h=3); }

//inside(50);

module bottle() {
    difference() {
        minkowski() { children(); wall(); }
        children();
    }
    info() children();
}

// simple cube
//cube(10);
//info() cube(10);



// make a bottle with s specific volume
// iter : number of iterations for binary search
// min, max : range of allowed parameter for bottle
// target is the target volume
module solve(iter=0,min=10,max=130,target=80) {
    m=(min+max)/2;
    probe(volume=true) {
        inside(m); // we measure the inside
        echo("iter=",iter," height=",m," volume=",volume);
        if( iter==0 ) {
            echo("bottle height=",m," volume=",volume);
            bottle() inside(m);
        }else{
            if( volume>target ) solve(iter-1,min,m,target);
            else solve(iter-1,m,max,target);
        }
    }
}

//solve(7,10,130,80*1000);


// bottle 2 range from 10= 24ml... to 30 = 131ml
//bottle() inside2(30);
//info() inside2(30);

module solve2(iter=0,min=10,max=130,target=80) {
    m=(min+max)/2;
    probe(volume=true) {
        inside2(m); // we measure the inside
        echo("iter=",iter," height=",m," volume=",volume);
        if( iter==0 ) {
            echo("bottle height=",m," volume=",volume);
            bottle() inside2(m);
        }else{
            if( volume>target ) solve2(iter-1,min,m,target);
            else solve2(iter-1,m,max,target);
        }
    }
}

//solve2(10,10,30,80*1000);


// bottle 3 range from 10= 33ml... to 35 = 140ml
//bottle() inside3(35);
//info() inside3(35);

module solve3(iter=0,min=10,max=130,target=80) {
    m=(min+max)/2;
    probe(volume=true) {
        inside3(m); // we measure the inside
        echo("iter=",iter," height=",m," volume=",volume);
        if( iter==0 ) {
            echo("bottle height=",m," volume=",volume);
            bottle() inside3(m);
        }else{
            if( volume>target ) solve3(iter-1,min,m,target);
            else solve3(iter-1,m,max,target);
        }
    }
}

// bottles with heights 10,50,90,130
for(i=[0:3]) { translate([i*70+60,0,0]) bottle() inside(10+i*40); }

// all bottles of volume 40,60,80,100 ml
for(i=[0:3]) { translate([i*70+60,-150,0]) solve(10,10,130,(40+i*20)*1000); }

// all bottles of volume 40,60,80,100 ml
for(i=[0:3]) { translate([i*60+60,-300,0]) solve2(12,10,30,(40+i*20)*1000); }

// all bottles of volume 40,60,80,100 ml
for(i=[0:3]) { translate([i*70+60,-450,0]) solve3(10,10,35,(40+i*20)*1000); }


////// un test de graphe...

/*
for(i=[0:60]) {
    probe(volume=true) {
        inside(i+20);
        echo(volume/1000);
        translate([i,0,0]) cube([1,1,volume/1000]);
    }
}
*/

