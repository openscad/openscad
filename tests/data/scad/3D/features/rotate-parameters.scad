s=[1,2,3];
//Fine
translate([0,0,0])
rotate(0)
cube(s);

translate([4,0,0])
rotate(45)
cube(s);

translate([8,0,0])
rotate(30,[0,1,0])
cube(s);

translate([12,0,0])
rotate([45])
cube(s);

translate([16,0,0])
rotate([15,30])
cube(s);

translate([20,0,0])
rotate([15,30,45])
cube(s);

//Edges cases
translate([-12,0,0])
{
    rotate(undef)
    rotate() //same as undef
    rotate([])
    cube(s);
}

//when deg_a is an array, the 'v' argument is ignored
translate([24,0,0])
rotate([45,30,15],v=[0,0,0])
cube(s);

//Problems with the parameters
//We mainly care that this calls create a warning
color("red")
translate([-6,0,0])
{
    rotate([45,45,45,45])
    cube(s);

    rotate()
    cube(s);

    rotate("45")
    cube(s);

    rotate(45,[0,1,0,0])
    cube(s);
    
    rotate(v=[1,0,0])
    cube(s);
    
    rotate("a","v")
    sphere();

    rotate(v="v",a="a")
    sphere();

    rotate(["a"],["v"])
    sphere();
}
