child(-1)test();

module childOutRange(num){
    child(5);
}

childOutRange(5)cube();
childOutRange(5){
    cube();
    sphere();
}

module lineup(num, space) {
   for (i = [0 : num-1])
     translate([ space*i, 0, 0 ]) children([0:10000]);
}

module testStr(){
    children("test");
}

lineup(1, 65){ sphere(30);cube(35);}

testStr()cube();