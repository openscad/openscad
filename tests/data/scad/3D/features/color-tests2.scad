// https://github.com/openscad/openscad/pull/5185#issuecomment-2183769802
rotate([0, 0, -90]) 
  difference() {
    color("yellow")
      render()
        difference() {
          color("red") sphere(10);
          color("blue") cube(10);
        }
    color("green")
      cylinder(r=3, h=20, center=true);
  }