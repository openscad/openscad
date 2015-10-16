module object() {
     difference() {
          cube(5);
          cube(3);
          translate ([0, 0, 2]) cube (2);
     }

     difference() {
          cube (5);
          cube (3);
     }
}

minkowski() {
     object();
     cube();
}
