module localfiles_submodule()
{
  linear_extrude(h=100) import("localfile.dxf");
  translate([-250,0,0]) linear_extrude(file="localfile.dxf");
  translate([0,350,0]) rotate_extrude(file="localfile.dxf");
  translate([250,0,0]) scale([200,200,50]) surface("localfile.dat");

  translate([0,-200,0]) sphere(r=dxf_dim(file="localfile.dxf", name="localfile")/2);
}
