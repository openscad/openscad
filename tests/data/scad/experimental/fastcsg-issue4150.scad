$fn=12;
cylinder(25, 5, center=true);
mirror([0, 0, 1])
{
  cylinder(h=2, r=8, center=true);
  cylinder(h=6, d=12, center=true);
}
