from openscad import *
from pylaser import *

obj=sphere(r=30)

lc = LaserCutter()
lc.add_volume(obj,4,4)
lc.link()

#lc.preview()
lc.finalize().rotz(-90).translate([20,100]).show()

#b.show()
