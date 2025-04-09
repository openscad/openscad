from openscad import *

leg = cube([30,1,10]) .front(0.5)
leg |= cylinder(d=3,h=10,fn=10)
leg -= cylinder(d=2,h=12,fn=10).down(1)

leg1=leg - cylinder(d=4,h=4).up(3)
leg2=leg  - cylinder(d=4,h=4).up(-0.5)
leg2 -= cylinder(d=4,h=4).up(6.5)

axis=cylinder(d=1.5,h=10)
hinge=axis | leg2 | leg1.rotate([0,0,"180"]) 

hinge.show()
