from pythonscad import *

base_style=4
diameter=28
thickness=3
taper=2
x_or_y = 90
if base_style % 2 == 0:
    x_or_y = x_or_y // ( base_style // 2 )

stretch_x = 200


x_scale = float(stretch_x) / 100.0;

y_top_scale = ( diameter - (2.0 * taper)  ) / diameter

x_top_scale = ( diameter * x_scale - (2.0 * taper)  ) / diameter

bottom = (circle(r=diameter/2, fn=base_style)
          .rotate([0,0,x_or_y])
          .scale([x_scale,1,1])
          )

print(x_top_scale, y_top_scale)

top = (circle(r=diameter/2, fn=base_style)
       .rotate([0,0,x_or_y])
       .scale([x_scale,1,1])
       .translate([0,0,thickness]))

fillet_mask = top.linear_extrude(3).scale([1.2,1.2,1]).down(2)

base = skin(bottom, top).fillet(3,fillet_mask)

#fillet_mask.show()
base.show()
