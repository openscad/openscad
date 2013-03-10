// bottom row = reference
// middle row = should match reference
// top row = should be inscribed in middle row in 'top' view

$fn=10;

color("red") {
translate([0, 0,-10]) cube(); 
translate([0,10,-10]) cube([5,1,1]);
translate([0,20,-10]) cube([1,6,1]);
translate([0,30,-10]) cube([1,1,7]);
translate([0,40,-10]) cube([5,6,1]); 
translate([0,60,-10]) cube([1,6,7]);
translate([0,50,-10]) cube([5,1,7]);
translate([0,70,-10]) cube([8,9,1]);
translate([0,80,-10]) cube([9,1,1]);
translate([0,90,-10]) cube([5,6,1]);
}

translate([0, 0,0]) cube(); 
translate([0,10,0]) resize([5,0,0]) cube(); 
translate([0,20,0]) resize([0,6,0]) cube(); 
translate([0,30,0]) resize([0,0,7]) cube(); 
translate([0,40,0]) resize([5,6,0]) cube(); 
translate([0,60,0]) resize([0,6,7]) cube(); 
translate([0,50,0]) resize([5,0,7]) cube(); 
translate([0,70,0]) resize([8,9]) cube(); 
translate([0,80,0]) resize([9]) cube(); 
translate([0,90,0]) resize([5,6,7]) cube(); 

color("blue"){
translate([0, 0,10]) cube(); 
translate([2.5,10.5,10]) resize([5,0,0]) sphere(0.5); 
translate([0.5,23,10]) resize([0,6,0]) sphere(0.5); 
translate([0.5,30.5,10]) resize([0,0,7]) sphere(0.5); 
translate([2.5,43,10]) resize([5,6,0]) sphere(0.5); 
translate([0.5,63,10]) resize([0,6,7]) sphere(0.5); 
translate([2.5,50.5,10]) resize([5,0,7]) sphere(0.5); 
translate([4,74.5,10]) resize([8,9]) sphere(0.5); 
translate([4.5,80.5,10]) resize([9]) sphere(0.5); 
translate([2.5,93,10]) resize([5,6,7]) sphere(0.5); 
}