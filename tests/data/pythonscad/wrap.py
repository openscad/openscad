from openscad import *

rR=3
fn=13

g=text("Hello world").linear_extrude(0.5).rotate([90,0,0]).roty(-6)
g=g.wrap(rR,fn=10).rotz(210)
g.show()
