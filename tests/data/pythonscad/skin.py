from openscad import *
a=square(4,center=True).roty(40)
b=circle(r=2,fn=20).rotx(40).up(10)
s=skin(a,b)
s.show()
