from openscad import *
from pylibfive import *
c=lv_coord()
s1=lv_sphere(lv_trans(c,[2,2,2]),2)
b1=lv_box(c,[2,2,2])
sdf=lv_union_stairs(s1,b1,1,3)
fobj=frep(sdf,[-4,-4,-4],[4,4,4],20)
fobj.show()