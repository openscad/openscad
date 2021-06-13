module donut() {
  rotate_extrude(convexity=2)
    translate([5,0,0])
      difference() {
        circle(r=2);
        circle(r=1);
      }
}

difference()
{
    donut();
    translate([-10,-10,0]) cube(10);
}
