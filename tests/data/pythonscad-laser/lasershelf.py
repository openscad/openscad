from openscad import *
from pylaser import *

obj=cylinder(r=78.0,h=27)


lc = LaserCutter()
lc.add_volume_tri(obj,5)
lc.link()

#lc.preview()

recipe=[(0,9,4),(2,10),(7,5),(6,3),(1,11),(8,13),(12,14)]
res=lc.finalize(recipe) 
(res + [51,17]).show()
