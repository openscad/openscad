from openscad import *

from pylaser import *

lc=LaserCutter(4)
lc.add_surface(cube([40,40,20]))
lc.link()

#lc.preview()
lc.finalize().show()
