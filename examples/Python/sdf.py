from openscad import *
from pylibfive import *

# Just assemble you libfive object first
c=lv_coord()
s1=lv_sphere(lv_trans(c,[2,2,2]),2)
b1=lv_box(c,[2,2,2])
sdf=lv_union_smooth(s1,b1,0.6)

# And mesh it finally to get something, that openSCAD can display
fobj=frep(sdf,[-4,-4,-4],[4,4,4],20)
fobj.show()

