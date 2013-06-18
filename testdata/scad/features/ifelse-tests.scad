if (true) {
  cube(2, true);
  translate([-3,0,0]) cube(2, true);
}
else {
  cylinder(r=1,h=2);
  translate([-3,0,0]) cylinder(r=1,h=2);
}

translate([3,0,0])
  if (false) cylinder(r=1,h=2);
  else cube(2, true);

translate([0,3,0])
  if (false) cylinder(r=1,h=2);
  else if (true) cube(2, true);
  else sphere();

translate([3,3,0])
  if (false) cylinder(r=1,h=2);
  else if (false) sphere();
  else cube(2, true);

translate([6,0,0])
  if (0) cylinder(r=1,h=2);
  else cube(2, true);

translate([6,3,0])
  if (1) cube(2, true);
  else cylinder(r=1,h=2);

translate([9,0,0])
  if ("") cylinder(r=1,h=2);
  else cube(2, true);

translate([9,3,0])
  if ("hello") cube(2, true);
  else cylinder(r=1,h=2);

translate([12,0,0])
  if ([]) cylinder(r=1,h=2);
  else cube(2, true);

translate([12,3,0])
  if ([1,2,3]) cube(2, true);
  else cylinder(r=1,h=2);

translate([15,0,0])
  if (ILLEGAL) cylinder(r=1,h=2);
  else cube(2, true);
