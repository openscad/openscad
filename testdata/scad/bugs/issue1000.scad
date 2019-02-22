//
// CSG rendering issue:
// This results in a blue sphere rather than a sphere consisting of red/blue half spheres
// The reason is likely that OpenCSG doesn't track
// which positive object is the source of what depth fragment and thus lets the last rendered object win.
//
// uncomment the render() calls to generate expected test results

color([1,0,0]) translate([0,0,0]) /*render()*/ intersection() {
  sphere(10);
  translate([0,0,10]) cube([20,20,20],center=true);
}

color([0,0,1]) /*render()*/ difference() { 
  sphere(10);
  translate([0,0,10]) cube([20,20,20],center=true);
}
