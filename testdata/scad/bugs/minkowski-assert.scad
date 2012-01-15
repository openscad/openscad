/*
  Originally reported by chrysn 20100516:

  terminate called after throwing an instance of 'CGAL::Assertion_exception'
   what():  CGAL ERROR: assertion violation!
   Expr: N.is_valid(0,0)
   File: /usr/include/CGAL/convex_decomposition_3.h
   Line: 113

   This appears to have been a CGAL issue which was fixe in CGAL-3.9.
*/

minkowski() {
 cube(20, center=true);
 rotate([20, 30, 40])
   difference() {
     cube(5, center=true);
     cube([1, 1, 10], center=true);
   }
}
