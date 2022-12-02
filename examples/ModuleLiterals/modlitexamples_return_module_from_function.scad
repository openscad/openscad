
fm = function(x) 
  let ( m = module(y,x1 = x) cube([x1,y,20]) )
m;
(fm(100))(50);  // instantiate the module returned from fm 

