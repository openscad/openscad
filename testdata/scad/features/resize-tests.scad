// bottom row (red) = reference
// middle row (gold) = should match reference
// top row (blue) = should be 'spherical' versions of gold row,
//     and should be inscribed in gold row in 'top' view
// back row (green) = should be all cubes auto-scaled up
// back top (purple) = uses 'auto' feature
// lime green = 'auto' feature where we 'shrink'
// pink = recursive resize, negative, <1, wrong syntax, etc

$fn=8;

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
translate([0,90,-10]) cube([5,6,7]);
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
translate([2.5,50.5,10]) resize([5,0,7]) sphere(0.5); 
translate([0.5,63,10]) resize([0,6,7]) sphere(0.5); 
translate([4,74.5,10]) resize([8,9]) sphere(0.5); 
translate([4.5,80.5,10]) resize([9]) sphere(0.5); 
translate([2.5,93,10]) resize([5,6,7]) sphere(0.5); 
}

color("green"){
translate([10, 0, 0]) cube(); 
translate([10,10,0]) resize([5,0,0],auto=true) cube(); 
translate([10,20,0]) resize([0,6,0],auto=true) cube(); 
translate([10,30,0]) resize([0,0,7],auto=true) cube(); 
translate([10,40,0]) resize([5,6,0],true) cube(); 
translate([10,50,0]) resize([5,0,7],true) cube(); 
translate([10,60,0]) resize([0,6,7],auto=true) cube(); 
translate([10,70,0]) resize([8,9],auto=true) cube(); 
translate([10,80,0]) resize([9],true) cube(); 
translate([10,90,0]) resize([5,6,7],auto=true) cube(); 
}

color("purple"){
translate([10, 0, 10]) cube(); 
translate([10,10,10]) resize([5,0,0],auto=[true,true,false]) cube();
translate([10,20,10]) resize([6,0,0],auto=[true,true,true]) cube(); 
translate([13.5,33.5,10]) resize([7,0,0],auto=[true,false,false]) sphere(); 
translate([10,40,10]) resize([6,0,0],auto=[true,false,true]) cube(); 
translate([10,50,10]) resize([7,0,7],auto=[false,true,true]) cube(); 
translate([13.5,63.5,10]) resize([7,0,0],auto=[false,true,false]) sphere(); 
translate([10,70,10]) resize([8,0,0],auto=[false,false,false]) cube(); 
translate([10,80,10]) resize([9,0,0],auto=[false,false,true]) cube(); 
translate([10,90,10]) resize([0,0,7],auto=[true,true,false]) cube();
}

color("pink"){
translate([10 , 0,-10]) resize([4,4,4]) resize([5000,100,1000]) cube();
translate([20,0,-10]) resize([-5,0,0]) cube(); 
translate([30,0,-10]) resize([-5,0,0],auto=3) cube(); 
translate([40,0,-10]) resize(-5,0,0,auto=3) cube(); 
translate([50,0,-10]) resize(5,0,0) cube(); 
translate([60,0,-10]) resize([0.5,0,7]) cube([0.5,1,1000]);
translate([70,0,-10]) resize([0,0,0.5]) cube([6,6,10000000000]);
}

color("lime"){ 
translate([20,0,0]) resize([5,0,0],auto=[true,true,false]) cube(9); 
translate([30,0,0]) resize([6,0,0],auto=[true,true,true]) cube(9); 
translate([40,0,0]) resize([6,0,0],auto=[true,false,true]) cube(9); 
translate([50,0,0]) resize([5,0,20],auto=[false,true,true]) cube(9); 
translate([60,0,0]) resize([0,0,7],auto=[false,false,true]) cube(9); 
translate([70,0,0]) resize([6,0,0],auto=[true,true,false]) cube(9);
}
