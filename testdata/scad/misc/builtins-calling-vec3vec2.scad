//Various lengths
cube();  //I'm fine 
cube(2); //I'm fine
cube([1]);
cube([1,2]);
cube([1,2,3]); //I'm fine
cube([1,2,3,4]);
cube([[1],2,3]);

//not numbers
cube([1,"test",3]); 
cube([1,"2",3]);
cube(["1.2","2",3]); 
cube("[1,2,3]");

square();
square(2); //I'm fine
square([1]);
square([1,2]);
square([1,2,3]);
square("[1,2]");

mirror()
mirror(1)
mirror([1,2])//I'm fine
mirror([1,2,3])//I'm fine
mirror([1,2,3,4])
mirror("[1,2,3]")

scale()
scale(1)//I'm fine
scale([1])
scale([1,2])  //I'm fine
scale([1,2,3])//I'm fine
scale([1,2,3,4])
scale("[1,2]")
cube([3,2,1]); //I'm fine

//here we have to decide how to handle it:
cube([1,-2,3]); 
cube([1/0,2,3]); 
