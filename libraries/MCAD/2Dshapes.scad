/*
 *  OpenSCAD 2D Shapes Library (www.openscad.org)
 *  Copyright (C) 2012 Peter Uithoven
 *
 *  License: LGPL 2.1 or later
*/

// 2D Shapes
//ngon(sides, radius, center=false);
//complexRoundSquare(size,rads1=[0,0], rads2=[0,0], rads3=[0,0], rads4=[0,0], center=true)
//roundedSquare(pos=[10,10],r=2)
//ellipsePart(width,height,numQuarters)
//donutSlice(innerSize,outerSize, start_angle, end_angle) 
//pieSlice(size, start_angle, end_angle) //size in radius(es)
//ellipse(width, height) {

// Examples
/*use <layouts.scad>;
grid(105,105,true,4)
{
	// ellipse
	ellipse(50,75);

	// part of ellipse (a number of quarters)
	ellipsePart(50,75,3);
	ellipsePart(50,75,2);
	ellipsePart(50,75,1);

	// complexRoundSquare examples
	complexRoundSquare([75,100],[20,10],[20,10],[20,10],[20,10]);
	complexRoundSquare([75,100],[0,0],[0,0],[30,50],[20,10]);
	complexRoundSquare([50,50],[10,20],[10,20],[10,20],[10,20],false);
	complexRoundSquare([100,100]);
	complexRoundSquare([100,100],rads1=[20,20],rads3=[20,20]);

	// pie slice
	pieSlice(50,0,10);
	pieSlice(50,45,190);
	pieSlice([50,20],180,270);

	// donut slice
	donutSlice(20,50,0,350);
	donutSlice(30,50,190,270);
	donutSlice([40,22],[50,30],180,270);
	donutSlice([50,20],50,180,270);
	donutSlice([20,30],[50,40],0,270);
}*/
//----------------------

// size, top left radius, top right radius, bottom right radius, bottom left radius, center
module complexRoundSquare(size,rads1=[0,0], rads2=[0,0], rads3=[0,0], rads4=[0,0], center=true)
{
	width = size[0];
	height = size[1];
	//%square(size=[width, height],center=true);
	x1 = 0-width/2+rads1[0];
	y1 = 0-height/2+rads1[1];
	x2 = width/2-rads2[0];
	y2 = 0-height/2+rads2[1];
	x3 = width/2-rads3[0];
	y3 = height/2-rads3[1];
	x4 = 0-width/2+rads4[0];
	y4 = height/2-rads4[1];

	scs = 0.1; //straight corner size

	x = (center)? 0: width/2;
	y = (center)? 0: height/2;

	translate([x,y,0])
	{
		hull() {
			// top left
			if(rads1[0] > 0 && rads1[1] > 0)
				translate([x1,y1]) mirror([1,0])		ellipsePart(rads1[0]*2,rads1[1]*2,1);
			else 
				translate([x1,y1]) 						square(size=[scs, scs]);
			
			// top right
			if(rads2[0] > 0 && rads2[1] > 0)
				translate([x2,y2]) 						ellipsePart(rads2[0]*2,rads2[1]*2,1);	
			else 
				translate([width/2-scs,0-height/2]) 	square(size=[scs, scs]);

			// bottom right
			if(rads3[0] > 0 && rads3[1] > 0)
				translate([x3,y3]) mirror([0,1]) 		ellipsePart(rads3[0]*2,rads3[1]*2,1);
			else 
				translate([width/2-scs,height/2-scs]) 	square(size=[scs, scs]);
			
			// bottom left
			if(rads4[0] > 0 && rads4[1] > 0)
				translate([x4,y4]) rotate([0,0,-180]) 	ellipsePart(rads4[0]*2,rads4[1]*2,1);
			else 
				#translate([x4,height/2-scs]) 	square(size=[scs, scs]);
		}
	}
}
module roundedSquare(pos=[10,10],r=2) {
	minkowski() {
		square([pos[0]-r*2,pos[1]-r*2],center=true);
		circle(r=r);
	}
}
// round shapes
// The orientation might change with the implementation of circle...
module ngon(sides, radius, center=false){
    rotate([0, 0, 360/sides/2]) circle(r=radius, $fn=sides, center=center);
}
module ellipsePart(width,height,numQuarters)
{
    o = 1; //slight overlap to fix a bug
	difference()
	{
		ellipse(width,height);
		if(numQuarters <= 3)
			translate([0-width/2-o,0-height/2-o,0]) square([width/2+o,height/2+o]);
		if(numQuarters <= 2)
			translate([0-width/2-o,-o,0]) square([width/2+o,height/2+o*2]);
		if(numQuarters < 2)
			translate([-o,0,0]) square([width/2+o*2,height/2+o]);
	}
}
module donutSlice(innerSize,outerSize, start_angle, end_angle) 
{   
    difference()
    {
        pieSlice(outerSize, start_angle, end_angle);
        if(len(innerSize) > 1) ellipse(innerSize[0]*2,innerSize[1]*2);
        else circle(innerSize);
    }
}
module pieSlice(size, start_angle, end_angle) //size in radius(es)
{	
    rx = ((len(size) > 1)? size[0] : size);
    ry = ((len(size) > 1)? size[1] : size);
    trx = rx* sqrt(2) + 1;
    try = ry* sqrt(2) + 1;
    a0 = (4 * start_angle + 0 * end_angle) / 4;
    a1 = (3 * start_angle + 1 * end_angle) / 4;
    a2 = (2 * start_angle + 2 * end_angle) / 4;
    a3 = (1 * start_angle + 3 * end_angle) / 4;
    a4 = (0 * start_angle + 4 * end_angle) / 4;
    if(end_angle > start_angle)
        intersection() {
		if(len(size) > 1)
        	ellipse(rx*2,ry*2);
		else
			circle(rx);
        polygon([
            [0,0],
            [trx * cos(a0), try * sin(a0)],
            [trx * cos(a1), try * sin(a1)],
            [trx * cos(a2), try * sin(a2)],
            [trx * cos(a3), try * sin(a3)],
            [trx * cos(a4), try * sin(a4)],
            [0,0]
       ]);
    }
}
module ellipse(width, height) {
  scale([1, height/width, 1]) circle(r=width/2);
}