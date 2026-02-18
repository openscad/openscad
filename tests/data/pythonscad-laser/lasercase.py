from openscad import *

from pylaser import *
box=cube([150,150,40])
f=box.faces()
lc=LaserCutter(4)
lc.add_surface(box)
lc.add_faces([square([150,40]).rotx(90).back(75)])
lc.add_faces([square([40,75]).roty(-90).right(75)])
lc.alter_face([0,0,1], lambda f: False) 
lc.link()

#lc.preview()
lc.finalize().show()