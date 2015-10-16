module object() {
     difference() {
          translate ([0, 0, 12]) cube(30, center=true);
          translate ([0, 0, 12]) cube ([20, 10, 10], center=true);
          translate ([0, 0, 3])  cylinder ( h=10, r1=8, r2=8, center=true);
     }
     
     difference() {
          translate ([0, 0, 0])  cube ([18,18,25], center=true);
          translate ([0, 0, 12]) cube ([20, 10, 10], center=true);
     }
}

minkowski() {
     object(); 
     cube(2);
}
