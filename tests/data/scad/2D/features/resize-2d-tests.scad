// red = reference
// gold = basic resize
// green = auto resize
// pink = errors, wrong syntax, trying to resize in 3rd dimension, etc

$fn=10;

// two simple holes
module shape(){
	difference() {
		square([5,5]);
		translate([1,1]) square();
		translate([3,3]) circle();
	}
}

// holes that have problems (duplicate vertex)
module shape2(){
	difference() {
		square([5,5]);
		translate([1,1]) square();
		translate([2,2]) square();
	}
}

// one square split into two by another
module shape3(){
	difference() {
		square([5,5]);
		translate([0,2.5]) square([5,1]);
	}
}

color("red") {
translate([-16,0]) scale([3,3]) shape();
translate([-16,16]) scale([3,3]) shape2();
translate([-16,32]) scale([3,3]) shape3();
}

translate([0,0]) resize([15,15]) shape();
translate([0,16]) resize([15,15,0]) shape2();
translate([0,32]) resize([15,15]) shape3();

color("green"){
translate([16,0]) resize([15,0],auto=false) scale([1,3]) shape();
translate([16,16]) resize([0,15],auto=true) scale() shape2();
translate([16,32]) resize([0,15],auto=[true,false]) shape3();
}

color("pink"){
translate([32,0]) resize([0,0],auto=[false,true]) shape();
translate([32,16]) resize([0,0,15],auto=true) shape2();
translate([32,32]) resize([0,0,15]) shape3();
}

color("blue"){
translate([-16,-16]) resize([10,8],auto=[false,true]) 
	scale([0.5,100,20]) shape();
translate([0,-16]) resize([8,10,15],auto=true) 
	scale([1000,0.5]) shape2();
translate([16,-16]) resize([10,8,15]) 
	scale([200,200]) shape3();
}
