
// modified from
// https://github.com/doug-moen/openscad2/blob/master/rfc/Objects.md#object-literals

lollipop = module {
  radius   = 10; // candy
  diameter = 3;  // stick
  height   = 50; // stick
  translate([0,0,height]) sphere(r=radius);
  cylinder(d=diameter,h=height);
};

lollipop();