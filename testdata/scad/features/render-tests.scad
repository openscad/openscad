module edgeprofile()
{
  difference() {
    cube([20, 20, 150], center = true);
    translate([-10, -10, 0]) cylinder(h = 80, r = 10, center = true);
  }
}

module rendered_edgeprofile()
{
  render(convexity = 2) edgeprofile();
}

module mycube() {
  difference() {
    cube(100, center = true);
    translate([ -50, -50, 0 ]) rotate(180, [0, 0, 1]) edgeprofile();
    translate([ 50, -50, 0 ]) rotate(270, [0, 0, 1]) edgeprofile();
  }
}

module rendered_mycube() {
  difference() {
    cube(100, center = true);
    translate([ -50, -50, 0 ]) rotate(180, [0, 0, 1]) rendered_edgeprofile();
    translate([ 50, -50, 0 ]) rotate(270, [0, 0, 1]) rendered_edgeprofile();
  }
}

// Empty
render();
// No children
render() { }

mycube();

translate([110,0,0]) rendered_mycube();

// This may trigger a convexity issue
translate([-110,0,0]) {
  difference() {
    render(convexity=2) difference() {
      cube(100, center = true);
      cylinder(r=30,h=110,center=true);
    }
    translate([-55, -55, 45]) cube(10);
  }
}
