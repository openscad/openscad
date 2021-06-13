module tree(currentScale, levels)
{
  h = currentScale;
  w = currentScale/5;
  childScale = currentScale * 0.7;
  
  if (levels > 0) {
    cylinder(r=w, h=h);
    translate([0,0,h]) for (i = [1:2]) {
      rotate([40, 0, i * 180]) tree(childScale, levels-1);
    }
  }
}

tree(1, 4);
