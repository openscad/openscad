RES=100;
Logo(50, RES);
colorValueR=246/255;
colorValueG=241/255;
colorValueB=200/255;

module Logo(size=50, $fn=100) {
    hole = size/2;
    cylinderHeight = size * 1.25;
    difference() {
        color ([39/255,84/255,139/255]) sphere(d=size);
        
        color ([168/255,177/255,202/255]) cylinder(d=hole, h=cylinderHeight, center=true);
        // The '#' operator highlights the object
        
        rotate([90, 0, 0]) color ([colorValueR,colorValueG,colorValueB]) cylinder(d=hole, h=cylinderHeight, center=true);
        
        rotate([0, 90, 0]) color ([168/255,177/255,202/255]) cylinder(d=hole, h=cylinderHeight, center=true);
        
        rotate([30,30,0]) color ([168/255,177/255,202/255]) star(cylinderHeight, 12, [2,5]);
        rotate([45,-30,0]) color ([168/255,177/255,202/255]) star(cylinderHeight, 14, [2,5]);
        rotate([90,0,45]) color ([168/255,177/255,202/255]) star(cylinderHeight, 16, [1.6,4]);
    }
    
    difference(){
        color ([colorValueR,colorValueG,colorValueB, .75])rotate([90, 0, 0]) cylinder(d=hole, h=54, center=true);
        color ([129/255,127/255,106/255, .85])translate ([6.5,0,0]) rotate([90, 0, 0]) cylinder(d=hole*0.8, h=cylinderHeight, center=true);
    }
}

module star(height, num, radii) {
    function r(a) = (floor(a / 10) % 2) ? 10 : 8;
    linear_extrude(height, center=true, convexity=3) polygon([for (i=[0:num-1], a=i*360/num, r=radii[i%len(radii)]) [ r*cos(a), r*sin(a) ]]);
}
