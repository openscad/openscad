circle(r=5);

translate([15,0,0]) square(size=[10,10], center=true);

translate([30,0,0]) polygon(points=[[-5,-5],[5,-5],[0,5]], paths=[[0,1,2]]);

translate([0,-15,0]) {
  difference() {
    circle(r=5);
    translate([0,-6,0]) square([12,12], center=true);
  }
}