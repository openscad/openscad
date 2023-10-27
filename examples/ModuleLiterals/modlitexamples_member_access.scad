
// modified from
// https://github.com/doug-moen/openscad2/blob/master/rfc/Objects.md#object-literals

lollipop = let(r = 10, d=3, h=50) {
  radius:   r, // candy
  diameter: d, // stick
  height:   h, // stick
  obj: {{
    translate([0,0,h]) sphere(r=r);
    cylinder(d=d,h=h);
  }}
};

echo(r = lollipop.radius);
echo(d = lollipop.diameter);
echo(h = lollipop.height);

lollipop.obj;
