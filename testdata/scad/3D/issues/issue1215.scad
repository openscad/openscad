difference() {
  rotate([90,0,90])
  linear_extrude(height=10)
  polygon(points=[[-4,0],[0,0],[0,3],[-3.5,3],[-4,2]]);

  rotate([90,0,0])
  linear_extrude(height=4)
  polygon(points=[[0,3],[0.5,3],[0,2]]);

}
